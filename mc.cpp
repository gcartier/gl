#include <GL/glew.h>
#include <GL/glfw.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
 
static const int width = 1024;   
static const int height = 768;
static const float PI = 3.1415926535897932384626433832f;
 
static std::string LoadShaderFromFile(const std::string& name)
{
    std::string ret, line;
    std::ifstream f(name.c_str());
    while(std::getline(f, line))
	ret += line + std::string("\n");
    return ret;
}
 
struct Vector3
{
    Vector3() : x(), y(), z(){}
    Vector3(float a, float b, float c) : x(a), y(b), z(c){}
    float x,y,z;
};
 
class Matrix4
{
public:
    Matrix4(){  zero(); }
    void zero(){ std::fill(data, data+16, 0.0f); }
    void identity(){ zero(); data[0] = data[5] = data[10] = data[15] = 1.0f; }
    float& operator[](size_t index){ return data[index]; }
    const float* c_ptr() const { return data; }
private:
    float data[16];
};
 
Matrix4 CreatePerspectiveProjection(float fov, float aspect, float near, float far)
{
    Matrix4 mat;
    float angle = (fov / 180.0f) * PI;
    float f = 1.0f / std::tan( angle * 0.5f );
 
    mat[0] = f / aspect;
    mat[5] = f;
    mat[10] = (far + near) / (near - far);
    mat[11] = -1.0f;
    mat[14] = (2.0f * far*near) / (near - far);
 
    return mat;
}
 
class Application   
{   
public:
    Application()
    {
	SetupShaders();
	SetupVertexArray();
	SetupBufferObject();
 
	int loc;
	Matrix4 proj = CreatePerspectiveProjection(90.0f, 4.0f/3.0f, 1.0f, 10.0f);
 
	glBindVertexArray(vertexArrayID);
	glUseProgram(progShader);
	loc = glGetAttribLocation(progShader, "posIn");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, 0);
	loc = glGetUniformLocation(progShader, "projectionMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, proj.c_ptr());
 
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
    }
private:
    void SetupShaders()
    {
	std::string shaderSource;
	const char* cstr;
 
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	geomShader = glCreateShader(GL_GEOMETRY_SHADER);
        fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	progShader = glCreateProgram();
 
	shaderSource = LoadShaderFromFile("vertex.shader");
	if(shaderSource.length() < 1){
	    printf("Empty vertex shader.\n");
	    exit(-1);
	}
	cstr = shaderSource.c_str();
	glShaderSource(vertexShader, 1, &cstr, NULL);
 
	shaderSource = LoadShaderFromFile("geometry.shader");
	if(shaderSource.length() < 1){
	    printf("Empty geometry shader.\n");
	    exit(-1);
	}
	cstr = shaderSource.c_str();
	glShaderSource(geomShader, 1, &cstr, NULL);
 
	shaderSource = LoadShaderFromFile("fragment.shader");
	if(shaderSource.length() < 1){
	    printf("Empty fragment shader.\n");
	    exit(-1);
	}
	cstr = shaderSource.c_str();
	glShaderSource(fragShader, 1, &cstr, NULL);
 
	glCompileShader(vertexShader);
        glCompileShader(geomShader);
	glCompileShader(fragShader);
	glAttachShader(progShader, vertexShader);
        /*Remember, geometry shaders are optional. Only used here for
          demonstration. It doesn't do anything interesting. */
        glAttachShader(progShader, geomShader);
	glAttachShader(progShader, fragShader);
	glLinkProgram(progShader);
 
	/* check for compiler or linker errors here */
    }
 
    void SetupVertexArray()
    {
	glGenVertexArrays(1, &vertexArrayID);
    }
 
    void SetupBufferObject()
    {
	std::vector<Vector3> dataArray(3);
	dataArray[0] = Vector3( 0.0f,  0.5f, -2.0f);
	dataArray[1] = Vector3(-0.5f, -0.5f, -2.0f);
	dataArray[2] = Vector3( 0.5f, -0.5f, -2.0f);
 
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vector3) * 3, &dataArray[0].x, GL_STATIC_DRAW);
    }
 
    GLuint GetShaderHandle() const { return progShader; }
 
    GLuint vertexShader, geomShader, fragShader, progShader;
    GLuint vertexArrayID;
    GLuint vertexBufferID;
 
};
 
void Render(Application& app);
 
 
int main(void)
{
    bool running = true;
    if(!glfwInit())
	return -1;
 
    glfwOpenWindowHint(GLFW_WINDOW_NO_RESIZE, GL_TRUE);
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
 
    /* If you use GLEW, comment these out, as GLEW seems
       to have some weird issues with forward-compatible core profiles.
       Otherwise you should always use forward-compatible core profiles
       in order to get proper error-reporting from OpenGL if you use outdated
       functionality that makes performance suffer.
       REMEMBER THAT GLEW IS NOT A PART OF OPENGL. YOU CAN SET UP THE
       FUNCTION-POINTERS YOURSELF */
    //glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
 
    /* Create a Window with a 32-bit RGBA color buffer, 24 bit depth buffer,
       8-bit stencil buffer and with double buffering */
    if(!glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 8, GLFW_WINDOW))
	return -1;
 
    glfwEnable(GLFW_STICKY_KEYS);
 
    if(glewInit())
	return -1;
 
    Application app;
 
    while(running){
	if(glfwGetKey(GLFW_KEY_ESC))
	    running = false;
	Render(app);
	glfwSwapBuffers();
    }
 
    glfwCloseWindow();
    glfwTerminate();
    return 0;
}
 
void Render(Application& app)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
