#include <windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#ifndef WGLCREATE
#include <GL/glfw.h>
#endif
#include <stdlib.h>
#include <stdio.h>


//-------
/// Log
//-------


FILE *logfile = NULL;


static void print(char* string)
{
	if (!logfile)
		logfile = fopen("log.txt", "w");
	
	fprintf(logfile, string);
	fflush(logfile);
}


static void vprint(char* format, va_list arguments)
{
	if (!logfile)
		logfile = fopen("log.txt", "w");
	
	vfprintf(logfile, format, arguments);
	fflush(logfile);
}


static void printlog(char* format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	vprint(format, arguments);
	print("\n");
	va_end(arguments);
}


static void printerror(char* format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	print("***** ");
	vprint(format, arguments);
	print(" *****\n");
	va_end(arguments);
}


//-----------
/// Shaders
//-----------


static const char* pVS = "							\n\
#version 130										\n\
													\n\
void main()											\n\
{													\n\
    gl_Position = gl_Vertex;						\n\
}";


static const char* pFS = "							\n\
#version 130										\n\
													\n\
void main()											\n\
{													\n\
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);		\n\
}";


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	printlog("Adding shader");
	
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        printerror("Error creating shader");
        exit(0);
    }

    const GLchar* p[1];
    p[0] = pShaderText;
    glShaderSource(ShaderObj, 1, p, NULL);
    glCompileShader(ShaderObj);
    GLint success;
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        printerror("Error compiling shader");
        exit(EXIT_FAILURE);
    }

    glAttachShader(ShaderProgram, ShaderObj);
}


//-----------
/// Objects
//-----------


GLuint VBO;


static void CreateVertexBuffer()
{
	const float Vertices[] = {
		-1.0f, -1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};

	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
}


//----------
/// Window
//----------


#ifdef WGLCREATE
HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

BOOL	keys[256];			// Array Used For The Keyboard Routine
BOOL	active=TRUE;		// Window Active Flag Set To TRUE By Default
BOOL	fullscreen=FALSE;	// Fullscreen Flag Not Set To Fullscreen Mode By Default

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc


GLvoid ResizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	printlog("Resize %d, %d", width, height);
	
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}


int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	return TRUE;										// Initialization Went OK
}


int DrawGLScene(GLvoid)									// Here Where We Do All The Drawing
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	glLoadIdentity();									// Reset The Current Modelview Matrix
	return TRUE;										// Everything Went OK
}


GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			printerror("Release of DC and RC failed");
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			printerror("Release rendering context failed");
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		printerror("Release device context failed");
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		printerror("Could not release window");
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass((LPCWSTR) "OpenGL", hInstance))			// Are We Able To Unregister Class
	{
		printerror("Could not unregister class");
		hInstance=NULL;									// Set hInstance To NULL
	}
}


void CreateGLWindow(char* title, int width, int height, BOOL fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= (LPCWSTR) "OpenGL";					// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		printerror("Failed to register the window class");
		exit(EXIT_FAILURE);
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory Is Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= 32;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			printerror("The requested fullscreen mode is not supported by your video card. Using windowed mode instead");
			fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		printlog("Fullscreen mode");
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		printlog("Windowed mode");
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								(LPCWSTR) "OpenGL",					// Class Name
								(LPCWSTR) title,					// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		printerror("Window creation error");
		exit(EXIT_FAILURE);
	}
	
	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		24,											// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		printerror("Could not create an OpenGL device context");
		exit(EXIT_FAILURE);
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		printerror("Could not find a suitable pixelformat");
		exit(EXIT_FAILURE);
	}

	if (!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		printerror("Could not set the pixelformat");
		exit(EXIT_FAILURE);
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		printerror("Could not create an OpenGL rendering context");
		exit(EXIT_FAILURE);
	}

	if (!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		printerror("Could not activate the OpenGL rendering context");
		exit(EXIT_FAILURE);
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ResizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		printerror("Initialization failed");
		exit(EXIT_FAILURE);
	}
}


LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ResizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
#endif


//--------
/// Main
//--------


#ifdef WGLCREATE
int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
#else
int main(void)
#endif
{
#ifdef WGLCREATE
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop
#else
	int running = GL_TRUE;
#endif
	
#ifdef WGLCREATE
	CreateGLWindow("OpenGL", 640, 480, fullscreen);
	printlog("Window created");
	
	HDC		pDC = GetDC(hWnd);
	HGLRC	m_hrc;
	
	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize  = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion   = 1;
	pfd.dwFlags    = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
 
	int nPixelFormat = ChoosePixelFormat(pDC, &pfd);
 
	if (nPixelFormat == 0)
		exit(EXIT_FAILURE);
 
	BOOL bResult = SetPixelFormat(pDC, nPixelFormat, &pfd);
 
	if (!bResult)
		exit(EXIT_FAILURE);
 
	HGLRC tempContext = wglCreateContext(pDC);
	wglMakeCurrent(pDC, tempContext);
	
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		printerror("Unable to initialize GLEW");
		exit(EXIT_FAILURE);
	}
	
	int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_FLAGS_ARB, 0,
		0
	};
 
	if (wglewIsSupported("WGL_ARB_create_context") == 1)
	{
		m_hrc = wglCreateContextAttribsARB(pDC, 0, attribs);
		wglMakeCurrent(NULL,NULL);
		wglDeleteContext(tempContext);
		wglMakeCurrent(pDC, m_hrc);
	}
	else
	{	// It is not possible to make a GL 3.x context. Use the old style context (GL 2.1 and before)
		m_hrc = tempContext;
	}
#else
	// Initialize GLFW
	if (!glfwInit())
	{
		exit(EXIT_FAILURE);
	}
	
	// Open an OpenGL window
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 0);
	if (!glfwOpenWindow(600, 400, 0, 0, 0, 0, 0, 0, GLFW_WINDOW)) // GLFW_FULLSCREEN
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	
	glfwSetWindowTitle("Hello world!");
	
    glewInit();
    if (glewIsSupported("GL_VERSION_3_0"))
        printlog("Ready for OpenGL 3.0");
    else {
        printerror("OpenGL 3.0 not supported");
        exit(EXIT_FAILURE);
    }
#endif
	//	GLuint vao;
	//	glGenVertexArrays(1, &vao);
	//	glBindVertexArray(vao);
    
    CreateVertexBuffer();
    
	GLuint p;
	
	// Create the program
	p = glCreateProgram();
	printlog("Program created");
	
    AddShader(p, pVS, GL_VERTEX_SHADER);
    AddShader(p, pFS, GL_FRAGMENT_SHADER);
	
	// Link and set program to use
	glLinkProgram(p);
	glUseProgram(p);

#ifdef WGLCREATE
	while (!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if (active)								// Program Active?
			{
				if (keys[VK_ESCAPE])				// Was ESC Pressed?
				{
					done=TRUE;						// ESC Signalled A Quit
				}
				else								// Not Time To Quit, Update Screen
				{
					DrawGLScene();					// Draw The Scene
					SwapBuffers(hDC);				// Swap Buffers (Double Buffering)
				}
			}

			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				CreateGLWindow("OpenGL", 640, 480, fullscreen);
				printlog("Window recreated");
			}
		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
#else
	// Main loop
	while (running)
	{
		// OpenGL rendering goes here...
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
	    glEnableVertexAttribArray(0);
	    glBindBuffer(GL_ARRAY_BUFFER, VBO);
	    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		
	    glDrawArrays(GL_TRIANGLES, 0, 3);
		
	    glDisableVertexAttribArray(0);
	
		// Swap front and back rendering buffers
		glfwSwapBuffers();
		
		// Check if ESC key was pressed or window was closed
		running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
	}
	
	// Close window and terminate GLFW
	glfwTerminate();
	
	// Exit program
	exit(EXIT_SUCCESS);
#endif
}
