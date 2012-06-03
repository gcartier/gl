#define GLEW_
#define OPENGL3_
//#define IMMEDIATE_


#include <windows.h>
#ifdef GLEW_
	#include <GL/glew.h>
	#include <GL/wglew.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif
#ifndef WGL_
	#include <GL/glfw.h>
#endif
#include <stdlib.h>
#include <stdio.h>


//-------
/// Todo
//-------


// - Is it realy necessary to recompile the shaders when switching fullscreen / windowed mode?


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


static void printexit(char* format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	print("***** ");
	vprint(format, arguments);
	print(" *****\n");
	va_end(arguments);
	
	exit(EXIT_FAILURE);
}


//-----------
/// Shaders
//-----------


#ifdef GLEW_
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


GLuint vertShader;
GLuint fragShader;


static GLuint CreateShader(const char* pShaderText, GLenum ShaderType)
{
	printlog("Creating shader");
	
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0)
		printexit("Error creating shader");

	const GLchar* p[1];
	p[0] = pShaderText;
	glShaderSource(ShaderObj, 1, p, NULL);
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		printexit("Error compiling shader");
	}

	return ShaderObj;
}


static void CreateShaders()
{
	vertShader = CreateShader(pVS, GL_VERTEX_SHADER);
	fragShader = CreateShader(pFS, GL_FRAGMENT_SHADER);
}


static void AddShaders(GLuint ShaderProgram)
{
	printlog("Adding shaders");
	
	glAttachShader(ShaderProgram, vertShader);
	glAttachShader(ShaderProgram, fragShader);
}
#endif


//-----------
/// Objects
//-----------


#ifdef GLEW_
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
#endif


//----------
/// Window
//----------


#ifdef WGL_
HWND		hWnd=NULL;		// Holds Our Window Handle
HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HINSTANCE	hInstance;		// Holds The Instance Of The Application

BOOL	keys[256];			// Array Used For The Keyboard Routine
BOOL	active=TRUE;		// Window Active Flag Set To TRUE By Default
BOOL	fullscreen=TRUE;	// Fullscreen Flag Not Set To Fullscreen Mode By Default

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc


GLvoid ResizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	printlog("Resize %d, %d", width, height);
	
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0, 0, width, height);					// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width/(GLfloat)height, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}


int SetupGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations
	return TRUE;										// Initialization Went OK
}


GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL, 0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL, NULL))				// Are We Able To Release The DC And RC Contexts?
			printexit("Release of DC and RC failed");

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
			printexit("Release rendering context failed");
		
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC)
	{
		if (!ReleaseDC(hWnd, hDC))						// Are We Able To Release The DC
			printexit("Release device context failed");
		
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd)
	{
		if (!DestroyWindow(hWnd))						// Are We Able To Destroy The Window?
			printexit("Could not release window");
		
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass(L"OpenGL", hInstance))			// Are We Able To Unregister Class
	{
		printexit("Could not unregister class");
		
		hInstance=NULL;									// Set hInstance To NULL
	}
}


static void printkill(char* format, ...)
{
	va_list arguments;

	va_start(arguments, format);
	print("***** ");
	vprint(format, arguments);
	print(" *****\n");
	va_end(arguments);
	
	KillGLWindow();
	exit(EXIT_FAILURE);
}


void CreateGLWindow(LPCWSTR title, int width, int height, BOOL fullscreenflag)
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
	wc.lpszClassName	= L"OpenGL";							// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
		printexit("Failed to register the window class");
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));	// Makes Sure Memory Is Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= 32;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
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
								L"OpenGL",							// Class Name
								title,								// Window Title
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
		printkill("Window creation error");
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
		printkill("Could not create an OpenGL device context");
	
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
		32,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
		printkill("Could not find a suitable pixelformat");

	if (!SetPixelFormat(hDC, PixelFormat, &pfd))	// Are We Able To Set The Pixel Format?
		printkill("Could not set the pixelformat");

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
		printkill("Could not create an OpenGL rendering context");

	if (!wglMakeCurrent(hDC, hRC))					// Try To Activate The Rendering Context
		printkill("Could not activate the OpenGL rendering context");

	ShowWindow(hWnd, SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ResizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!SetupGL())									// Setup Our Newly Created GL Window
		printkill("Initialization failed");
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
			ResizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif


//-----------
/// Context
//-----------


#ifdef WGL_
static void AdjustGLContext()
{
	#ifdef OPENGL3_
	if (wglewIsSupported("WGL_ARB_create_context") == 1)
	{
		int attribs[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 0,
			WGL_CONTEXT_FLAGS_ARB, 0,
			0
		};
		
		HGLRC	newRC;
	
		newRC = wglCreateContextAttribsARB(hDC, 0, attribs);
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hRC);
		wglMakeCurrent(hDC, newRC);
		hRC = newRC;
		printlog("OpenGL context recreated");
	}
	#endif
}
#endif


//----------
/// Render
//----------


void RenderScene()
{
#ifdef IMMEDIATE_
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glTranslatef(-1.5f,0.0f,-6.0f);						// Move Left 1.5 Units And Into The Screen 6.0
	glBegin(GL_TRIANGLES);								// Drawing Using Triangles
		glColor3f(1.0f,0.0f,0.0f);						// Set The Color To Red
		glVertex3f( 0.0f, 1.0f, 0.0f);					// Top
		glColor3f(0.0f,1.0f,0.0f);						// Set The Color To Green
		glVertex3f(-1.0f,-1.0f, 0.0f);					// Bottom Left
		glColor3f(0.0f,0.0f,1.0f);						// Set The Color To Blue
		glVertex3f( 1.0f,-1.0f, 0.0f);					// Bottom Right
	glEnd();											// Finished Drawing The Triangle
	glTranslatef(3.0f,0.0f,0.0f);						// Move Right 3 Units
	glColor3f(0.5f,0.5f,1.0f);							// Set The Color To Blue One Time Only
	glBegin(GL_QUADS);									// Draw A Quad
		glVertex3f(-1.0f, 1.0f, 0.0f);					// Top Left
		glVertex3f( 1.0f, 1.0f, 0.0f);					// Top Right
		glVertex3f( 1.0f,-1.0f, 0.0f);					// Bottom Right
		glVertex3f(-1.0f,-1.0f, 0.0f);					// Bottom Left
	glEnd();											// Done Drawing The Quad
#else
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	
	glDrawArrays(GL_TRIANGLES, 0, 3);
	
	glDisableVertexAttribArray(0);
#endif
}


//-----------
/// Program
//-----------


GLuint idProgram;


static void CreateProgram()
{
	//	GLuint vao;
	//	glGenVertexArrays(1, &vao);
	//	glBindVertexArray(vao);
	
	CreateVertexBuffer();
	
	GLuint p;
	
	// Create the program
	p = glCreateProgram();
	printlog("Program created");
	
	AddShaders(p);
	
	// Link and set program to use
	glLinkProgram(p);
	glUseProgram(p);
	
	idProgram = p;
}


static void ReleaseProgram()
{
	glDeleteProgram(idProgram);
}


//--------
/// Main
//--------


#ifdef WGL_
int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
#else
int main(void)
#endif
{
#ifdef WGL_
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop
#else
	int running = GL_TRUE;
#endif
	
#ifdef WGL_
	CreateGLWindow(L"OpenGL", 640, 480, fullscreen);
	printlog("Window created");
	
	#ifdef GLEW_
	GLenum err = glewInit();
	if (GLEW_OK != err)
		printexit("Unable to initialize GLEW");
	printlog("GLEW initialized");
	
	AdjustGLContext();
	#endif
#else
	// Initialize GLFW
	if (!glfwInit())
		exit(EXIT_FAILURE);
	printlog("GLFW initialized");
	
	// Open an OpenGL window
	#ifdef OPENGL3_
	glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 0);
	#endif
	if (!glfwOpenWindow(600, 400, 0, 0, 0, 0, 0, 0, GLFW_WINDOW)) // GLFW_FULLSCREEN
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	
	glfwSetWindowTitle("Hello world from GLFW!");
	
	#ifdef GLEW_
	glewInit();
	printlog("GLEW initialized");
	if (glewIsSupported("GL_VERSION_3_0"))
		printlog("Ready for OpenGL 3.0");
	else
		printexit("OpenGL 3.0 not supported");
	#endif
#endif

#ifdef GLEW_
#ifndef IMMEDIATE_
	CreateShaders();
	CreateProgram();
#endif
#endif

#ifdef WGL_
	while (!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))	// Is There A Message Waiting?
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
			// Draw The Scene. Watch For ESC Key And Quit Messages
			if (active)								// Program Active?
			{
				if (keys[VK_ESCAPE])				// Was ESC Pressed?
				{
					done=TRUE;						// ESC Signalled A Quit
				}
				else								// Not Time To Quit, Update Screen
				{
					RenderScene();					// Render The Scene
					SwapBuffers(hDC);				// Swap Buffers (Double Buffering)
				}
			}

			// Toggle Fullscreen / Windowed Mode
			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				CreateGLWindow(L"OpenGL", 640, 480, fullscreen);
				printlog("Window recreated");
				#ifdef GLEW_
				AdjustGLContext();
				#ifndef IMMEDIATE_
				ReleaseProgram();
				CreateShaders();
				CreateProgram();
				#endif
				#endif
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
		// Render
		RenderScene();
		
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
