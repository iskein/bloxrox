#include<bits/stdc++.h>
#include <iostream>
#include <fstream>
#include<mpg123.h>

#ifdef WIN32
#include<windows.h>
#endif
#include<FTGL/ftgl.h>
#include <GL/glew.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
typedef pair<int,int> ii;
#define rep(i,a, b) for(int i=(a); i<(b); i++)

void initGL(int width, int height);
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct FTGLFont
{
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
}GL3Font;

GLuint programID, fontProgramID, textureProgramID;
int gameOver = 0;
struct base_parameters
{
	int x,y;
	char type;
	VAO* object;
};

int rx = -1,ry = -1;
int axis = 0;
int moves = 0;
int level = 1;
int view = 0;
int px = 0, py = 5;
int switchOn = 0;
int blockarr[10][10];
int blockState = 1;
float scaleFactor = 1.0F;
float radius = 10.0f;
float elevation_angle = 90.0f;
float azimuthal_angle = 0.0f;
float cx = 0.0f,cy = 0.0f,cz = 10.0f;
char base[20][20];

map<pair<int, int>, base_parameters> base_map;


/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

glm::vec3 getRGBfromHue(int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2) + fracp - 1.0);
	if(hue < 60)
		return glm::vec3(1,x,0);
	else if(hue < 120)
		return glm::vec3(x,1,0);
	else  if(hue < 180)
		return glm::vec3(0,1,x);
	else if(hue < 240)return glm::vec3(0,x,1);
	else if(hue < 300)return glm::vec3(x,0,1);
	else return glm::vec3(1,0,x);
}
/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

//float triangle_rot_dir = 1;
//float rectangle_rot_dir = 1;
//bool triangle_rot_status = true;
//bool rectangle_rot_status = true;

static const GLfloat color_buffer_data_boundary [] = {
	0,0,0,
	0,0,0,
	0,0,0,


	0,0,0,
	0,0,0,
	0,0,0
};

static const GLfloat vertex_buffer_data_boundary [] = {
	0.5, 0.5, 0,
	-0.5, 0.5, 0,
	-0.5, -0.5, 0,

	-0.5, -0.5, 0,
	0.5, -0.5, 0,
	0.5, 0.5, 0
};

static const GLfloat color_buffer_data_B [] = {
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,

	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309,
	0.184, 0.309, 0.309
};

static const GLfloat color_buffer_data_L [] = {
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,

	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,

	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,

	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,

	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752,
	0.752, 0.752, 0.752
};
static const GLfloat color_buffer_data_F [] = {
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,

	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,

	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,

	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,

	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,

	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	
	1, 0.498, 0.313,
	1, 0.498, 0.313,
	1, 0.498, 0.313
};

static const GLfloat color_buffer_data_S [] = {
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,

	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,

	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,

	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,

	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,

	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0,	
	
	1, 0.843, 0,	
	1, 0.843, 0,	
	1, 0.843, 0
};
static const GLfloat vertex_buffer_data [] = {
	-0.5, -0.5, 0.2,
	0.5, 0.5, 0.2,
	-0.5, 0.5, 0.2,

	-0.5,-0.5, 0.2,
	0.5, -0.5, 0.2,
	0.5, 0.5, 0.2,

	-0.5, -0.5, -0.2,
	0.5, 0.5, -0.2,
	-0.5, 0.5, -0.2,

	-0.5,-0.5, -0.2,
	0.5, -0.5, -0.2,
	0.5, 0.5, -0.2,

	-0.5, 0.5, 0.2,
        0.5, 0.5, -0.2,
	-0.5, 0.5, -0.2,

	-0.5, 0.5, 0.2,
	0.5, 0.5, 0.2,
	0.5, 0.5, -0.2,
	
	0.5, 0.5, 0.2,
	0.5, -0.5, 0.2,
	0.5, 0.5, -0.2,

	0.5, -0.5, 0.2,
	0.5, -0.5, -0.2,
	-0.5, -0.5, -0.2,
		
	0.5, -0.5, 0.2,
	-0.5, -0.5, -0.2,
	-0.5, -0.5, 0.2,
	
	-0.5, 0.5, 0.2,
	-0.5, -0.5, 0.2,
	-0.5, 0.5, -0.2,

	-0.5, -0.5, 0.2,
	-0.5, -0.5, -0.2,
	-0.5, 0.5, -0.2	
};

static const GLfloat vertex_buffer_data_block [] = {
	-0.5, -0.5, 1,
	0.5, 0.5, 1,
	-0.5, 0.5, 1,

	-0.5,-0.5, 1,
	0.5, -0.5, 1,
	0.5, 0.5, 1,

	-0.5, -0.5, -1,
	0.5, 0.5, -1,
	-0.5, 0.5, -1,

	-0.5,-0.5, -1,
	0.5, -0.5, -1,
	0.5, 0.5, -1,

	-0.5, 0.5, 1,
        0.5, 0.5, -1,
	-0.5, 0.5, -1,

	-0.5, 0.5, 1,
	0.5, 0.5, 1,
	0.5, 0.5, -1,
	
	0.5, 0.5, 1,
	0.5, -0.5, 1,
	0.5, 0.5, -1,

	0.5, -0.5, 1,
	0.5, -0.5, -1,
	-0.5, -0.5, -1,
		
	0.5, -0.5, 1,
	-0.5, -0.5, -1,
	-0.5, -0.5, 1,
	
	-0.5, 0.5, 1,
	-0.5, -0.5, 1,
	-0.5, 0.5, -1,

	-0.5, -0.5, 1,
	-0.5, -0.5, -1,
	-0.5, 0.5, -1	
};

static const GLfloat color_buffer_data_block [] = {
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0,	
	
	1, 0, 0,	
	1, 0, 0,	
	1, 0, 0		
};	

static const GLfloat vertex_buffer_data_cube [] = {
	-0.5, -0.5, 0.5,
	0.5, 0.5, 0.5,
	-0.5, 0.5, 0.5,

	-0.5,-0.5, 0.5,
	0.5, -0.5, 0.5,
	0.5, 0.5, 0.5,

	-0.5, -0.5, -0.5,
	0.5, 0.5, -0.5,
	-0.5, 0.5, -0.5,

	-0.5,-0.5, -0.5,
	0.5, -0.5, -0.5,
	0.5, 0.5, -0.5,

	-0.5, 0.5, 0.5,
        0.5, 0.5, -0.5,
	-0.5, 0.5, -0.5,

	-0.5, 0.5, 0.5,
	0.5, 0.5, 0.5,
	0.5, 0.5, -0.5,
	
	0.5, 0.5, 0.5,
	0.5, -0.5, 0.5,
	0.5, 0.5, -0.5,

	0.5, -0.5, 0.5,
	0.5, -0.5, -0.5,
	-0.5, -0.5, -0.5,
		
	0.5, -0.5, 0.5,
	-0.5, -0.5, -0.5,
	-0.5, -0.5, 0.5,
	
	-0.5, 0.5, 0.5,
	-0.5, -0.5, 0.5,
	-0.5, 0.5, -0.5,

	-0.5, -0.5, 0.5,
	-0.5, -0.5, -0.5,
	-0.5, 0.5, -0.5	
};
void zoomIn()
{
	scaleFactor -= 0.2f;
	if(scaleFactor < 0.4f)scaleFactor = 0.4f;
}
void zoomOut()
{
	scaleFactor += 0.2f;
	if(scaleFactor > 2.0f)scaleFactor = 2.0f;
}

int win = 0;
void move(int state)
{
	moves += 1;
	int cnt = 0;
	rep(i,0,10)rep(j,0,10)cnt += blockarr[i][j];
	int x=-1,y=-1;
	rep(i,0,10)
	{
		rep(j,0,10)
			if(blockarr[i][j] == 1){x = i; y = j; break;}
		if(x != -1)break;
	}
	if(cnt == 1)
	{
		if(base[x][y] == 'W')win = 1;
		blockarr[x][y] = 0;
		if(state == 1)
		{
			if(y+1 > 9 || y+2 > 9)
			{
				gameOver = 1;
			}
			else
			{
				axis = 0;
				rx = x + 0.5;
				ry = y;
				blockarr[x][y+1] = 1;
				blockarr[x][y+2] = 1;
			}
		}
		else if(state == -1)
		{
			if(y-2 < 0 || y-1 < 0)
			{
				gameOver = 1;
			}
			else
			{
				axis = 0;
				rx = x - 0.5;
				ry = y;
				blockarr[x][y-1] = 1;
				blockarr[x][y-2] = 1;
			}
		}
		else if(state == 2)
		{
			if(x-1 < 0 || x-2 < 0)
			{
				gameOver = 1;
			}
			else
			{
				axis = 1;
				rx = x;
				ry = y + 0.5;
				blockarr[x-1][y] = 1;
				blockarr[x-2][y] = 1;
			}
		}
		else if(state == -2)
		{
			if(x+2 > 9 || x+1 > 9)
			{
				gameOver = 1;
			}
			else
			{
				axis = 1;
				rx = x;
				ry = y - 0.5;
				blockarr[x+1][y] = 1;
				blockarr[x+2][y] = 1;
			}
		}
	}
	else if(cnt == 2)
	{
		if(y+1 <= 9 && blockarr[x][y+1] == 1)
		{
			blockarr[x][y] = 0;
			blockarr[x][y+1] = 0;
			if(state == 1)
			{
				axis = 0;
				rx = x + 0.5;
				ry = y + 1;
				if(y+2 > 9)gameOver = 1;
				else blockarr[x][y+2] = 1;
			}
			if(state == -1)
			{
				axis = 0;
				rx = x - 0.5;
				ry = y;
				if(y-1 < 0)gameOver = 1;
				else blockarr[x][y-1] = 1;
			}
			if(state == 2)
			{
				axis = 1;
				rx = x;
				ry = y + 0.5;
				if(x-1 < 0 || y +1 > 9)gameOver = 1;
				else
				{
					blockarr[x-1][y] = 1;
					blockarr[x-1][y+1] = 1;
				}
			}
			if(state == -2)
			{
				axis = 1;
				rx = x;
				ry = y - 0.5;
				if(x+1 > 9 || y+1 > 9)gameOver = 1;
				else
				{
					blockarr[x+1][y] = 1;
					blockarr[x+1][y+1] = 1;
				}
			}
		}
		else if(y-1 >= 0 && blockarr[x][y-1] == 1)
		{
			blockarr[x][y] = 0;
			blockarr[x][y-1] = 0;
			if(state == 1)
			{
				axis = 0;
				rx = x + 0.5;
				ry = y;
				if(y+1 > 9)gameOver = 1;
				else blockarr[x][y+1] = 1;
			}
			if(state == -1)
			{
				axis = 0;
				rx = x - 0.5;
				ry = y - 1;
				if(y-2 < 0)gameOver = 1;
				else blockarr[x][y-2] = 1;
			}
			if(state == 2)
			{
				if(x-1 < 0 || y-1 < 0)gameOver = 1;
				else
				{
					blockarr[x-1][y] = 1;
					blockarr[x-1][y-1] = 1;
				}
			}
			if(state == -2)
			{
				if(x+1 > 9 || y-1 < 0)
					gameOver = 1;
				else
				{
					blockarr[x+1][y] = 1;
					blockarr[x+1][y-1] = 1;
				}
			}
		}
		else if(x+1 <= 9 && blockarr[x+1][y] == 1)
		{
			blockarr[x][y] = 0;
			blockarr[x+1][y] = 0;
			if(state == 1)
			{
				if(x+1 > 9 || y+1 > 9)gameOver = 1;
				else
				{
					blockarr[x][y+1] = 1;
					blockarr[x+1][y+1] = 1;
				}
			}
			if(state == -1)
			{
				if(x+1 > 9 || y-1 < 0)gameOver = 1;
				else
				{
					blockarr[x][y-1] = 1;
					blockarr[x+1][y-1] = 1;
				}
			}
			if(state == 2)
			{
				if(x-1 < 0)gameOver = 1;
				else blockarr[x-1][y] = 1;
			}
			if(state == -2)
			{
				if(x+2 > 9)gameOver = 1;
				else blockarr[x+2][y] = 1;
			}
		}
		else if(x-1 >= 0 && blockarr[x-1][y] == 1)
		{
			blockarr[x][y] = 0;
			blockarr[x-1][y] = 0;
			if(state == 1)
			{
				if(y+1 > 9 || x-1 < 0)gameOver = 1;
				else
				{
					blockarr[x][y+1] = 1;
					blockarr[x-1][y+1] = 1;
				}
			}
			if(state == -1)
			{
				if(y-1 < 0 || x-1 < 0)gameOver = 1;
				else
				{
					blockarr[x][y-1] = 1;
					blockarr[x-1][y-1] = 1;
				}
			}
			if(state == 2)
			{
				if(x-2 < 0)gameOver = 1;
				else blockarr[x-2][y] = 1;
			}
			if(state == -2)
			{
				if(x+1 > 9)gameOver = 1;
				else blockarr[x+1][y] = 1;
			}
			
		}
		
	}
}
int dirObject = 0;
/* Executed when a regular key is pressed */
void keyboardDown (unsigned char key, int x, int y)
{
    switch (key) {
        case 'Q':
        case 'q':
        case 27: //ESC
            exit (0);
	    break;
	case 'w':
	case 'W':
	    move(1);
	    break;
	case 's':
	case 'S':
	    move(-1);
	    break;
	case 'd':
	case 'D':
	    move(-2);
	    break;
	case 'a':
	case 'A':
	    move(2);
	    break;
	case 'B':
	case 'b':
	    elevation_angle = 90;
	    azimuthal_angle = 0;
	    view = 0;
	    break;
	case 't':
	case 'T':
	    elevation_angle = 30;
	    azimuthal_angle = 45;
	    view = 0;
	    break;
	case 'F':
	case 'f':
	    view = 1;
	    break;
	case 'O':
	case 'o':
	    dirObject = (dirObject + 1)%4;
	    view = 2;
	    break;
        default:
            break;
    }
}

/* Executed when a regular key is released */
void keyboardUp (unsigned char key, int x, int y)
{
    switch (key) {
        case 'c':
        case 'C':
            //rectangle_rot_status = !rectangle_rot_status;
            break;
        case 'p':
        case 'P':
            //triangle_rot_status = !triangle_rot_status;
            break;
        case 'x':
            // do something
            break;
        default:
            break;
    }
}

/* Executed when a special key is pressed */
void keyboardSpecialDown (int key, int x, int y)
{
}

/* Executed when a special key is released */
void keyboardSpecialUp (int key, int x, int y)
{
}

/* Executed when a mouse button 'button' is put into state 'state'
 at screen position ('x', 'y')
 */
void mouseClick (int button, int state, int x, int y)
{
	if(button == 3)
	{
		zoomIn();
	}
	else if(button  == 4)
	{
		zoomOut();
	}
}

/* Executed when the mouse moves to position ('x', 'y') */

float prevX = 0, prevY = 0;

void changeAngle(float x1, float y1, float x2, float y2)
{
	elevation_angle += ((y1 - y2)/20.0)*70;
	azimuthal_angle += ((x1 - x2)/20.0)*180;
	if(elevation_angle > 90)elevation_angle = 90;
	if(azimuthal_angle > 180)azimuthal_angle = 180;
	if(elevation_angle < 20)elevation_angle = 20;
	if(azimuthal_angle < -180)azimuthal_angle = -180;
	//cout << elevation_angle << " " << azimuthal_angle << endl;
}
void mouseMotion (int x, int y)
{
	float x1,y1;
	x1 = ((x-300.0)/300.0)*10;
	y1 = ((300.0 - y)/300.0)*10;
	changeAngle(x1,y1,prevX, prevY);
	prevX = x1;
	prevY = y1;
	//cout << x1 << " " << y1 << endl;
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (int width, int height)
{
	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) width, (GLsizei) height);

	// set the projection matrix as perspective/ortho
	// Store the projection matrix in a variable for future use

    // Perspective projection for 3D views

    // Ortho projection for 2D views
    //Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *boundary_arr[101][101], *map_arr[101][101], *block, *cube1, *cube2;

void createCube1()
{
	cube1 = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data_cube, color_buffer_data_block, GL_FILL);
}

void createCube2()
{
	cube2 = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data_cube, color_buffer_data_block, GL_FILL);
}

void createBlock()
{
	block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data_block, color_buffer_data_block, GL_FILL);
}
void createBase(int x, int y, char type)
{
	if(type == 'W' || type == 'E')return ;
	if(type == 'B')
		map_arr[x][y] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data_B, GL_FILL);
	if(type == 'F')
		map_arr[x][y] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data_F, GL_FILL);
	if(type == 'L')
		map_arr[x][y] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data_L, GL_FILL);
	if(type == 'S')
		map_arr[x][y] = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data_S, GL_FILL);

}
void createBoundary(int x, int y, char type)
{
	if(type == 'W'  || type == 'E')return ;
	boundary_arr[x][y] = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data_boundary, color_buffer_data_boundary, GL_LINE);
}
// Creates the triangle object used in this sample code
/*
void createTriangle ()
{
 * ONLY vertices between the bounds specified in glm::ortho will be visible on screen 

 * Define vertex array as used in glBegin (GL_TRIANGLES) 
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}
*/

//rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
float m = 0.0;
int gx = -1,gy = -1;
int fragile = 0;
void draw ()
{
     	if(view == 2 || view == 1)
		Matrices.projection = glm::perspective (90.0f, (GLfloat) 600 / (GLfloat) 600, 0.1f, 500.0f);
	else
    		Matrices.projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 500.0f);
	int px = -1, py = -1;
	int pwx = -1, pwy = -1;
	rep(i,0,10)rep(j,0,10)if(base[i][j] == 'W'){pwx = i; pwy = j;}
	rep(i,0,10)rep(j,0,10)if(blockarr[i][j] == 1){px = i;py = j;}
  
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  //glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );

  glm::vec3 eye0 ( (radius*cos(elevation_angle*M_PI/180.0f))*(cos(azimuthal_angle*M_PI/180.0f)), (radius*cos(elevation_angle*M_PI/180.0f))*(sin(azimuthal_angle*M_PI/180.0f)), radius*(sin(elevation_angle*M_PI/180.0f)) );
  //glm::vec3 eye ( cx, cy, cz );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target0 (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  //glm::vec3 up (0, 1, 0);
  glm::vec3 up (0, 0, 1);

  glm::vec3 eye1(px - 5, py - 5 - 3 ,2.5);
  glm::vec3 target1(px - 5,py - 5,1);
 
  int r = 10;
  int nor = 5;
  glm::vec3 eye2(px - 5 , py - 5  ,5-m);
  glm::vec3 target20(px - nor,py - nor + r ,1);
  glm::vec3 target21(px - nor + r,py - nor ,1);
  glm::vec3 target22(px - nor ,py - nor - r ,1);
  glm::vec3 target23(px - nor - r,py - nor ,1);

  // Compute Camera matrix (view)
  if(view == 0)
  	Matrices.view = glm::lookAt( eye0, target0, up ); // Rotating Camera for 3D
  else if(view == 1)
	  Matrices.view = glm::lookAt(eye1, target1, up);
  else if(view == 2)
  {
	  if(dirObject == 0)Matrices.view = glm::lookAt(eye2, target20, up);
	  if(dirObject == 1)Matrices.view = glm::lookAt(eye2, target21, up);
	  if(dirObject == 2)Matrices.view = glm::lookAt(eye2, target22, up);
	  if(dirObject == 3)Matrices.view = glm::lookAt(eye2, target23, up);
  }
  //  Don't change unless you are sure!!
  //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  // Change made here to change camera view
  //Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  //Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

  //glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  //glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  //glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  //Matrices.model *= triangleTransform; 
  //MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  //glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);


  if(gameOver == 1)
  {
	  // Enter Code for ending game herea
	  m += 0.1;
	  if(m > 10)initGL(600,600);
  }

  for(int i=0; i<10; i++)
  {
  	for(int j=0; j<10; j++)
	{
		if(base[i][j] == 'W' || base[i][j] == 'E')continue;
		if(base[i][j] == 'L' && switchOn == 0)continue;
  		Matrices.model = glm::mat4(1.0f);
  		glm::mat4 translateBase = glm::translate (glm::vec3(i-5, j-5,0));
  		glm::mat4 translateBaseF = glm::translate (glm::vec3(i-5, j-5,-m));
		glm::mat4 scaleBase = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  		Matrices.model *= (scaleBase);
		if(fragile == 1 && i == gx && j == gy)
  			Matrices.model *= (translateBaseF);
		else Matrices.model *= (translateBase);
  		MVP = VP * Matrices.model;
  		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  		draw3DObject(map_arr[i][j]);	
	}
  }
  rep(i,0,10)
  {
  	rep(j,0,10)
	{
		if(base[i][j] == 'W' || base[i][j] == 'E')continue;
  		Matrices.model = glm::mat4(1.0f);
  		glm::mat4 translateBase = glm::translate (glm::vec3(i-5, j-5,0.205));
  		glm::mat4 translateBaseF = glm::translate (glm::vec3(i-5, j-5,-m + 0.205));

		glm::mat4 scaleBase = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  		Matrices.model *= (scaleBase);
		if(fragile == 1 && i == gx && j == gy)
  			Matrices.model *= (translateBaseF);
		else Matrices.model *= (translateBase);
  		MVP = VP * Matrices.model;
  		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  		draw3DObject(boundary_arr[i][j]);	
	}	
  }	  
  int cnt = 0;
  rep(i,0,10)rep(j,0,10)cnt += blockarr[i][j];
  if(cnt == 1)
  {
	rep(i,0,10)rep(j,0,10)if(base[i][j] == 'S' && blockarr[i][j] == 1)switchOn = 1;
	int x = -1, y=-1;
  	rep(i,0,10)
	{
		rep(j,0,10)
		{
			if(blockarr[i][j] == 1)
			{
				x = i;
				y = j;
				break;
			}
		}
		if(x != -1)break;
	}
	if(base[x][y] == 'L' && switchOn == 0)gameOver = 1;
	if(base[x][y] == 'E')gameOver = 1;
	if(base[x][y] == 'W' && gameOver == 0){gameOver = 1; level += 1;}
  	if(base[x][y] == 'F')
	{
		gx = x;
		gy = y;
		fragile = 1;
		gameOver = 1;
	}
	Matrices.model = glm::mat4(1.0f);
  	glm::mat4 translateBlock = glm::translate (glm::vec3(x-5, y-5, 1.2 - m));        // glTranslatef
  	glm::mat4 scaleBlock = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  	Matrices.model *= (scaleBlock);
  	Matrices.model *= (translateBlock);
  	MVP = VP * Matrices.model;
  	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  	draw3DObject(block);
  }
  else if(cnt == 2)
  {
  	int x = -1; 
	int y = -1;
  	rep(i,0,10)
	{
		rep(j,0,10)
		{
			if(blockarr[i][j] == 1)
			{
				x = i;
				y = j;
				break;
			}
		}
		if(x != -1)break;
	}
	int nx, ny;
	if(x+1 <= 9 && blockarr[x+1][y] == 1){nx = x+1; ny = y;}
	if(x-1 >= 0 && blockarr[x-1][y] == 1){nx = x-1; ny = y;}
	if(y+1 <= 9 && blockarr[x][y+1] == 1){nx = x; ny = y+1;}
	if(y-1 >= 0 && blockarr[x][y-1] == 1){nx = x; ny = y-1;}
	if((base[x][y] == 'E') || (base[x][y] == 'F' && switchOn == 0) || base[nx][ny] == 'E')
		gameOver = 1;
  	
  		//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model = glm::mat4(1.0f);
  	glm::mat4 translateCube1 = glm::translate (glm::vec3(x-5, y-5, 0.7-m));
	glm::mat4 scaleCube1 = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));

  	Matrices.model *= (scaleCube1);
  	Matrices.model *= (translateCube1);
  	MVP = VP * Matrices.model;
  	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  	draw3DObject(cube1);	
	
	Matrices.model = glm::mat4(1.0f);
  	glm::mat4 translateCube2 = glm::translate (glm::vec3(nx-5, ny-5, 0.7-m));
  	glm::mat4 scaleCube2 = glm::scale(glm::vec3(scaleFactor, scaleFactor, scaleFactor));
  	Matrices.model *= (scaleCube2);
  	Matrices.model *= (translateCube2);
  	MVP = VP * Matrices.model;
  	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  	draw3DObject(cube2);

  }

  float fontScaleValue = 4;
  glm::vec3 fontColor = glm::vec3(float(22)/255.0, float(162)/255.0, float(133)/255.0);

  glUseProgram(fontProgramID);
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0));

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateText = glm::translate(glm::vec3(-10,5,0));
  glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue, fontScaleValue, fontScaleValue));
  Matrices.model *= (translateText * scaleText);
  MVP = Matrices.projection * Matrices.view * Matrices.model;
  glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
  glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

  char s[5]; 
  char revs[5];
  int d = 0;
  int temp1 = moves;
  while(temp1 > 0)
  {
  	revs[d] = (char)(temp1%10 + '0');
	temp1 = temp1/10;
	d += 1;
  }
  revs[d] = '\0';
  int a = 0;
  for(int i=d-1; i >= 0; i--)
  {
  	s[a] = revs[i];
	a += 1;
  }
  s[a] = '\0';
  char moves[100] = {'M','o','v','e','s',' ',':',' '};
  GL3Font.font -> Render(strcat(moves,s));

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateText1 = glm::translate(glm::vec3(-10,7,0));
  glm::mat4 scaleText1 = glm::scale(glm::vec3(fontScaleValue, fontScaleValue, fontScaleValue));
  Matrices.model *= (translateText1 * scaleText1);
  MVP = Matrices.projection * Matrices.view * Matrices.model;
  glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
  glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);

  char s1[5]; 
  char revs1[5];
  d = 0;
  temp1 = level;
  while(temp1 > 0)
  {
  	revs1[d] = (char)(temp1%10 + '0');
	temp1 = temp1/10;
	d += 1;
  }
  revs1[d] = '\0';
  a = 0;
  for(int i=d-1; i >= 0; i--)
  {
  	s1[a] = revs1[i];
	a += 1;
  }
  s1[a] = '\0';
  char level[100] = {'L','e','v','e','L',' ',':',' '};
  GL3Font.font -> Render(strcat(level,s1));


  // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject draws the VAO given to it using current MVP matrix
/*
  draw3DObject(triangle);

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
  glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle * rotateRectangle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(rectangle);
*/
  // Swap the frame buffers
  glutSwapBuffers ();

  glutPostRedisplay();
  // Increment angles

  //camera_rotation_angle++; // Simulating camera rotation
  //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Executed when the program is idle (no I/O activity) */
void idle () {
    // OpenGL should never stop drawing
    // can draw the same scene or a modified scene
    draw (); // drawing same scene
}


/* Initialise glut window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
void initGLUT (int& argc, char** argv, int width, int height)
{
    // Init glut
    glutInit (&argc, argv);

    // Init glut window
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitContextVersion (3, 3); // Init GL 3.3
    glutInitContextFlags (GLUT_CORE_PROFILE); // Use Core profile - older functions are deprecated
    glutInitWindowSize (width, height);
    glutCreateWindow ("Sample OpenGL3.3 Application");

    // Initialize GLEW, Needed in Core profile
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cout << "Error: Failed to initialise GLEW : "<< glewGetErrorString(err) << endl;
        exit (1);
    }

    // register glut callbacks
    glutKeyboardFunc (keyboardDown);
    glutKeyboardUpFunc (keyboardUp);

    glutSpecialFunc (keyboardSpecialDown);
    glutSpecialUpFunc (keyboardSpecialUp);

    glutMouseFunc (mouseClick);
    glutMotionFunc (mouseMotion);

    glutReshapeFunc (reshapeWindow);

    glutDisplayFunc (draw); // function to draw when active
    glutIdleFunc (idle); // function to draw when idle (no I/O activity)
    
    glutIgnoreKeyRepeat (true); // Ignore keys held down
}

/* Process menu option 'op' */
void menu(int op)
{
    switch(op)
    {
        case 'Q':
        case 'q':
            exit(0);
    }
}

void addGLUTMenus ()
{
    // create sub menus
    int subMenu = glutCreateMenu (menu);
    glutAddMenuEntry ("Do Nothing", 0);
    glutAddMenuEntry ("Really Quit", 'q');

    // create main "middle click" menu
    glutCreateMenu (menu);
    glutAddSubMenu ("Sub Menu", subMenu);
    glutAddMenuEntry ("Quit", 'q');
    glutAttachMenu (GLUT_MIDDLE_BUTTON);
}


/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (int width, int height)
{
	switchOn = 0;
	gameOver = 0;
	m = 0;
	axis = -1;
	rx = -1;
	ry = -1;
	cout << "level is : " << level << endl;
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	string line;
	int top = 0;
	ifstream myfile;
	if(level == 1)myfile.open("map0.txt");
	if(level == 2)myfile.open("map1.txt");
	if(level == 3)myfile.open("map2.txt");
	if(myfile.is_open())
	{
		while(getline (myfile, line))
		{
			for(int i=0; i<10; i++)
			{
				base[top][i] = line[i];
			}
			top += 1;
		}
		myfile.close();
	}
	else cout << "Unable to open map" << endl;
	
	for(int i=0; i<10; i++)
	{
		for(int j=0; j<10; j++)
		{
			blockarr[i][j] = 0;
		}
	}
	blockarr[5][0] = 1;
	for(int i=0; i<10;i++)
	{
		for(int j=0;j<10;j++)
		{
			if(base[i][j] == 'W' || base[i][j] == 'E')continue;
			createBase(i,j,base[i][j]);
		}
	}
	for(int i=0; i<10;i++)
	{
		for(int j=0;j<10;j++)
		{
			if(base[i][j] == 'W' || base[i][j] == 'E')continue;
			createBoundary(i,j,base[i][j]);
		}
	}
	createBlock();
	createCube1();
	createCube2();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (width, height);

	// Background color of the scene
	glClearColor (1.0f, 1.0f, 1.0f, 1.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile);
	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font";
		exit(EXIT_FAILURE);
	}

	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");
	
	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0,0);
	GL3Font.font->CharMap(ft_encoding_unicode);
	//createRectangle ();

	cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
	cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
	cout << "VERSION: " << glGetString(GL_VERSION) << endl;
	cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

    initGLUT (argc, argv, width, height);

    addGLUTMenus ();

	initGL (width, height);

    glutMainLoop ();

    return 0;
}
