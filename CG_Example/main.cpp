#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <glm.hpp>
#include "gtc/matrix_transform.hpp"
#include "vec3.hpp"
#include <AntTweakBar.h>

using namespace std;

#define BUFFER_OFFSET(i) ((void*)(i))

struct TVertex_VC
{
	float	x, y, z;
	float	nx, ny, nz;
	int	m;
};

struct triangle{
	TVertex_VC v0, v1, v2;
	glm::vec3 face;
};

struct material{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	float	shine;
};

struct light{
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec4 position;
	GLboolean on;
};

GLuint NumTris, material_count, NumV;
GLfloat x_min = FLT_MIN, x_max = FLT_MIN,
y_min = FLT_MIN, y_max = FLT_MIN,
z_min = FLT_MIN, z_max = FLT_MIN; //To calculate the model position
GLfloat global_r, global_g, global_b;
bool dLightr;
triangle *Tris;
material *Mate;
light sLight, dLight;
TwBar *bar;

GLushort	*pindex_triangle;
std::vector<TVertex_VC>	pvertex_triangle;
GLuint VAOID, IBOID, VBOID;
GLuint Shader, VertShader, FragShader;
enum Model { CUBE, COW, PHONE };
Model model = PHONE;
enum Orientation { CCW, CW} ;
Orientation ori = CCW;
enum Shading {FLAT, SMOOTH};
Shading sm = SMOOTH;
enum Rotation{X, Y, Z};
Rotation r = X;

void TW_CALL Reset(void *clientDate);
// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
// size of file returned in fSize
std::string loadFile(const char *fname)
{
	std::ifstream file(fname);
	if (!file.is_open())
	{
		cout << "Unable to open file " << fname << endl;
		exit(1);
	}

	std::stringstream fileData;
	fileData << file.rdbuf();
	file.close();

	return fileData.str();
}


// printShaderInfoLog
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
void printShaderInfoLog(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		// error check for fail to allocate memory omitted
		glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
		cout << "InfoLog : " << endl << infoLog << endl;
		delete[] infoLog;
	}
}

void InitGLStates()
{
	glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glReadBuffer(GL_BACK);
	glDrawBuffer(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(TRUE);
	glDisable(GL_STENCIL_TEST);
	glStencilMask(0xFFFFFFFF);
	glStencilFunc(GL_EQUAL, 0x00000000, 0x00000001);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);
	glClearStencil(0);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DITHER);
	TwInit(TW_OPENGL_CORE, NULL);

	glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	TwGLUTModifiersFunc(glutGetModifiers);
}

void InitializeUI(){
	bar = TwNewBar("TweakBar");
	TwDefine(" TweakBar size='200 400' color='96 216 224' "); // change default tweak bar size and color

	{
		TwEnumVal modelEV[3] = { { CUBE, "Cube" }, { COW, "Cow" }, { PHONE, "Phone" } };
		TwType modelType = TwDefineEnum("modelType", modelEV, 3);
		TwAddVarRW(bar, "Model", modelType, &model, " keyIncr = '<' keyDecr = '>'");
	}

	{
		TwEnumVal oriEV[2] = { { CW, "CW" }, { CCW, "CCW" } };
		TwType oriType = TwDefineEnum("oriType", oriEV, 2);
		TwAddVarRW(bar, "Orientation", oriType, &ori, " keyIncr = '<' keyDecr = '>'");
	}

	{
		TwEnumVal smEV[2] = { { FLAT, "FLAT" }, { SMOOTH, "SMOOTH" } };
		TwType smType = TwDefineEnum("smType", smEV, 2);
		TwAddVarRW(bar, "Shade Mode", smType, &sm, " keyIncr = '<' keyDecr = '>'");
	}

	{
		TwAddVarRW(bar, "gRed", TW_TYPE_FLOAT, &global_r, "group = Global_Light"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "gGreen", TW_TYPE_FLOAT, &global_g, "group = Global_Light"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "gBlue", TW_TYPE_FLOAT, &global_b, "group = Global_Light"
			" min=0 max=1.0 step=0.1");
	}

	{
		TwAddVarRW(bar, "sSwitch", TW_TYPE_BOOL32, &sLight.on, "group = Static_Light");

		TwAddVarRW(bar, "saRed", TW_TYPE_FLOAT, &sLight.ambient.r, "group = sAmbient"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "saGreen", TW_TYPE_FLOAT, &sLight.ambient.g, "group = sAmbient"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "saBlue", TW_TYPE_FLOAT, &sLight.ambient.b, "group = sAmbient"
			" min=0 max=1.0 step=0.1");

		TwAddVarRW(bar, "sdRed", TW_TYPE_FLOAT, &sLight.diffuse.r, "group = sDiffuse"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "sdGreen", TW_TYPE_FLOAT, &sLight.diffuse.g, "group = sDiffuse"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "sdBlue", TW_TYPE_FLOAT, &sLight.diffuse.b, "group = sDiffuse"
			" min=0 max=1.0 step=0.1");

		TwAddVarRW(bar, "ssRed", TW_TYPE_FLOAT, &sLight.specular.r, "group = sSpecular"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "ssGreen", TW_TYPE_FLOAT, &sLight.specular.g, "group = sSpecular"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "ssBlue", TW_TYPE_FLOAT, &sLight.specular.b, "group = sSpecular"
			" min=0 max=1.0 step=0.1");

		TwDefine("TweakBar/sAmbient group = Static_Light");
		TwDefine("TweakBar/sDiffuse group = Static_Light");
		TwDefine("TweakBar/sSpecular group = Static_Light");
	}

	{
		TwAddVarRW(bar, "dSwitch", TW_TYPE_BOOL32, &dLight.on, "group = Dynamic_Light");

		TwAddVarRW(bar, "daRed", TW_TYPE_FLOAT, &dLight.ambient.r, "group = dAmbient"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "daGreen", TW_TYPE_FLOAT, &dLight.ambient.g, "group = dAmbient"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "daBlue", TW_TYPE_FLOAT, &dLight.ambient.b, "group = dAmbient"
			" min=0 max=1.0 step=0.1");

		TwAddVarRW(bar, "ddRed", TW_TYPE_FLOAT, &dLight.diffuse.r, "group = dDiffuse"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "ddGreen", TW_TYPE_FLOAT, &dLight.diffuse.g, "group = dDiffuse"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "ddBlue", TW_TYPE_FLOAT, &dLight.diffuse.b, "group = dDiffuse"
			" min=0 max=1.0 step=0.1");

		TwAddVarRW(bar, "dsRed", TW_TYPE_FLOAT, &dLight.specular.r, "group = dSpecular"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "dsGreen", TW_TYPE_FLOAT, &dLight.specular.g, "group = dSpecular"
			" min=0 max=1.0 step=0.1");
		TwAddVarRW(bar, "dsBlue", TW_TYPE_FLOAT, &dLight.specular.b, "group = dSpecular"
			" min=0 max=1.0 step=0.1");

		TwAddVarRW(bar, "Rotation", TW_TYPE_BOOL8, &dLightr, "group = Dynamic_Light");
		{
			TwEnumVal rotEV[3] = { { X, "X-Axis" }, { Y, "Y-Axis" }, { Z, "Z-Axis" } };
			TwType rotDir = TwDefineEnum("rotDir", rotEV, 3);
			TwAddVarRW(bar, "Direction", rotDir, &r, " group = Dynamic_Light");
		}
		TwAddButton(bar, "Reset", Reset, NULL, " group = Dynamic_Light");

		TwDefine("TweakBar/dAmbient group = Dynamic_Light");
		TwDefine("TweakBar/dDiffuse group = Dynamic_Light");
		TwDefine("TweakBar/dSpecular group = Dynamic_Light");
	}
}

//Return the minimum value of three values
GLfloat min3(GLfloat x, GLfloat y, GLfloat z){
	return x < y ? (x < z ? x : z) : (y < z ? y : z);
}

//Return the maximum value of three values
GLfloat max3(GLfloat x, GLfloat y, GLfloat z){
	return x > y ? (x > z ? x : z) : (y > z ? y : z);
}

bool readin(char *FileName) {
	const int MAX_MATERIAL_COUNT = 255;
	glm::vec3 ambient[MAX_MATERIAL_COUNT], diffuse[MAX_MATERIAL_COUNT], specular[MAX_MATERIAL_COUNT];
	GLfloat x_temp, y_temp, z_temp;
	GLfloat shine[MAX_MATERIAL_COUNT];
	char ch;

	FILE* fp = fopen(FileName, "r");
	if (fp == NULL){
		printf(FileName, "doesn't exist");
		exit(1);
	}

	fscanf(fp, "%c", &ch);
	while (ch != '\n')
		fscanf(fp, "%c", &ch);

	fscanf(fp, "# triangles = %d\n", &NumTris);
	fscanf(fp, "Material count = %d\n", &material_count);

	NumV = NumTris * 3;
	pindex_triangle = new GLushort[NumV];
	Mate = new material[material_count];
	for (int i = 0; i < material_count; i++){
		fscanf(fp, "ambient color %f %f %f\n", &(Mate[i].ambient.x), &(Mate[i].ambient.y), &(Mate[i].ambient.z));
		fscanf(fp, "diffuse color %f %f %f\n", &(Mate[i].diffuse.x), &(Mate[i].diffuse.y), &(Mate[i].diffuse.z));
		fscanf(fp, "specular color %f %f %f\n", &(Mate[i].specular.x), &(Mate[i].specular.y), &(Mate[i].specular.z));
		fscanf(fp, "material shine %f\n", &(Mate[i].shine));
	}

	fscanf(fp, "%c", &ch);
	while (ch != '\n') // skip documentation line
		fscanf(fp, "%c", &ch);

	Tris = new triangle[NumTris];
	pvertex_triangle.clear();
	for (int i = 0; i<NumTris; i++) // read triangles
	{
		fscanf(fp, "v0 %f %f %f %f %f %f %d\n",
			&(Tris[i].v0.x), &(Tris[i].v0.y), &(Tris[i].v0.z),
			&(Tris[i].v0.nx), &(Tris[i].v0.ny), &(Tris[i].v0.nz),
			&(Tris[i].v0.m));
		fscanf(fp, "v1 %f %f %f %f %f %f %d\n",
			&(Tris[i].v1.x), &(Tris[i].v1.y), &(Tris[i].v1.z),
			&(Tris[i].v1.nx), &(Tris[i].v1.ny), &(Tris[i].v1.nz),
			&(Tris[i].v1.m));
		fscanf(fp, "v2 %f %f %f %f %f %f %d\n",
			&(Tris[i].v2.x), &(Tris[i].v2.y), &(Tris[i].v2.z),
			&(Tris[i].v2.nx), &(Tris[i].v2.ny), &(Tris[i].v2.nz),
			&(Tris[i].v2.m));
		fscanf(fp, "face normal %f %f %f\n", &(Tris[i].face.x), &(Tris[i].face.y),
			&(Tris[i].face.z));

	//Get the minimum and maximum value to calculate the position
		x_temp = min3(Tris[i].v0.x, Tris[i].v1.x, Tris[i].v2.x);
		y_temp = min3(Tris[i].v0.y, Tris[i].v1.y, Tris[i].v2.y);
		z_temp = min3(Tris[i].v0.z, Tris[i].v1.z, Tris[i].v2.z);
		x_min = x_min < x_temp ? x_min : x_temp;
		y_min = y_min < y_temp ? y_min : y_temp;
		z_min = z_min < z_temp ? z_min : z_temp;

		x_temp = max3(Tris[i].v0.x, Tris[i].v1.x, Tris[i].v2.x);
		y_temp = max3(Tris[i].v0.y, Tris[i].v1.y, Tris[i].v2.y);
		z_temp = max3(Tris[i].v0.z, Tris[i].v1.z, Tris[i].v2.z);
		x_min = x_min > x_temp ? x_min : x_temp;
		y_min = y_min > y_temp ? y_min : y_temp;
		z_min = z_min > z_temp ? z_min : z_temp;

		pvertex_triangle.push_back(Tris[i].v0);
		pvertex_triangle.push_back(Tris[i].v1);
		pvertex_triangle.push_back(Tris[i].v2);
	}

	fclose(fp);
	return true;
}

int LoadShader(const char *pfilePath_vs, const char *pfilePath_fs, bool bindTexCoord0, bool bindNormal, bool bindColor, GLuint &shaderProgram, GLuint &vertexShader, GLuint &fragmentShader)
{
	shaderProgram = 0;
	vertexShader = 0;
	fragmentShader = 0;

	// load shaders & get length of each
	int vlen;
	int flen;
	std::string vertexShaderString = loadFile(pfilePath_vs);
	std::string fragmentShaderString = loadFile(pfilePath_fs);
	vlen = vertexShaderString.length();
	flen = fragmentShaderString.length();

	if (vertexShaderString.empty())
	{
		return -1;
	}

	if (fragmentShaderString.empty())
	{
		return -1;
	}

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	const char *vertexShaderCStr = vertexShaderString.c_str();
	const char *fragmentShaderCStr = fragmentShaderString.c_str();
	glShaderSource(vertexShader, 1, (const GLchar **)&vertexShaderCStr, &vlen);
	glShaderSource(fragmentShader, 1, (const GLchar **)&fragmentShaderCStr, &flen);

	GLint compiled;

	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == FALSE)
	{
		cout << "Vertex shader not compiled." << endl;
		printShaderInfoLog(vertexShader);

		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;

		return -1;
	}

	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compiled);
	if (compiled == FALSE)
	{
		cout << "Fragment shader not compiled." << endl;
		printShaderInfoLog(fragmentShader);

		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;

		return -1;
	}

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	glBindAttribLocation(shaderProgram, 0, "InVertex");

	if (bindTexCoord0)
		glBindAttribLocation(shaderProgram, 1, "InTexCoord0");

	if (bindNormal)
		glBindAttribLocation(shaderProgram, 2, "InNormal");

	if (bindColor)
		glBindAttribLocation(shaderProgram, 3, "InColor");

	glLinkProgram(shaderProgram);

	GLint IsLinked;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, (GLint *)&IsLinked);
	if (IsLinked == FALSE)
	{
		cout << "Failed to link shader." << endl;

		GLint maxLength;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
		if (maxLength>0)
		{
			char *pLinkInfoLog = new char[maxLength];
			glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, pLinkInfoLog);
			cout << pLinkInfoLog << endl;
			delete[] pLinkInfoLog;
		}

		glDetachShader(shaderProgram, vertexShader);
		glDetachShader(shaderProgram, fragmentShader);
		glDeleteShader(vertexShader);
		vertexShader = 0;
		glDeleteShader(fragmentShader);
		fragmentShader = 0;
		glDeleteProgram(shaderProgram);
		shaderProgram = 0;

		return -1;
	}

	return 1;		//Success
}

void SetUniform(int programID, glm::vec3 camPos, glm::mat4 ModelMatrix, glm::mat4 ViewMatrix, glm::mat4	MVPMatrix, light sLight, light dLight){
	//MVP
	glUniformMatrix4fv(glGetUniformLocation(Shader, "MVP"), 1, FALSE, &MVPMatrix[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(Shader, "M"), 1, FALSE, &ModelMatrix[0][0]);

	//Materials
	if (model == CUBE || model == COW){
		glUniform3f(glGetUniformLocation(programID, "mat[0].ambient"), Mate[0].ambient.r, Mate[0].ambient.g, Mate[0].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[0].diffuse"), Mate[0].diffuse.r, Mate[0].diffuse.g, Mate[0].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[0].specular"), Mate[0].specular.r, Mate[0].specular.g, Mate[0].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[0].shine"), Mate[0].shine);
	}

	if (model == PHONE){
		//1st
		glUniform3f(glGetUniformLocation(programID, "mat[0].ambient"), Mate[0].ambient.r, Mate[0].ambient.g, Mate[0].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[0].diffuse"), Mate[0].diffuse.r, Mate[0].diffuse.g, Mate[0].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[0].specular"), Mate[0].specular.r, Mate[0].specular.g, Mate[0].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[0].shine"), Mate[0].shine);
		//2nd
		glUniform3f(glGetUniformLocation(programID, "mat[1].ambient"), Mate[1].ambient.r, Mate[1].ambient.g, Mate[1].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[1].diffuse"), Mate[1].diffuse.r, Mate[1].diffuse.g, Mate[1].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[1].specular"), Mate[1].specular.r, Mate[1].specular.g, Mate[1].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[1].shine"), Mate[1].shine);
		//3rd
		glUniform3f(glGetUniformLocation(programID, "mat[2].ambient"), Mate[2].ambient.r, Mate[2].ambient.g, Mate[2].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[2].diffuse"), Mate[2].diffuse.r, Mate[2].diffuse.g, Mate[2].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[2].specular"), Mate[2].specular.r, Mate[2].specular.g, Mate[2].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[2].shine"), Mate[2].shine);
		//4th
		glUniform3f(glGetUniformLocation(programID, "mat[3].ambient"), Mate[3].ambient.r, Mate[3].ambient.g, Mate[3].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[3].diffuse"), Mate[3].diffuse.r, Mate[3].diffuse.g, Mate[3].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[3].specular"), Mate[3].specular.r, Mate[3].specular.g, Mate[3].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[3].shine"), Mate[3].shine);
		//5th
		glUniform3f(glGetUniformLocation(programID, "mat[4].ambient"), Mate[4].ambient.r, Mate[4].ambient.g, Mate[4].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[4].diffuse"), Mate[4].diffuse.r, Mate[4].diffuse.g, Mate[4].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[4].specular"), Mate[4].specular.r, Mate[4].specular.g, Mate[4].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[4].shine"), Mate[4].shine);
		//6th
		glUniform3f(glGetUniformLocation(programID, "mat[5].ambient"), Mate[5].ambient.r, Mate[5].ambient.g, Mate[5].ambient.b);
		glUniform3f(glGetUniformLocation(programID, "mat[5].diffuse"), Mate[5].diffuse.r, Mate[5].diffuse.g, Mate[5].diffuse.b);
		glUniform3f(glGetUniformLocation(programID, "mat[5].specular"), Mate[5].specular.r, Mate[5].specular.g, Mate[5].specular.b);
		glUniform1f(glGetUniformLocation(programID, "mat[5].shine"), Mate[5].shine);
	}
}

void CreateGeometry(){

	for (int i = 0; i < NumV; i++){
		pindex_triangle[i] = i;
	}

	//Create the IBO for the triangle
	//16 bit indices
	//We could have actually made one big IBO for both the quad and triangle.
	glGenBuffers(1, &IBOID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NumV * sizeof(GLushort), pindex_triangle, GL_STATIC_DRAW);

	GLenum error = glGetError();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	error = glGetError();

	//Create VBO for the triangle
	glGenBuffers(1, &VBOID);

	glBindBuffer(GL_ARRAY_BUFFER, VBOID);
	glBufferData(GL_ARRAY_BUFFER, NumV * sizeof(TVertex_VC), &pvertex_triangle[0], GL_STATIC_DRAW);

	//Just testing
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	error = glGetError();

	//Second VAO setup *******************
	//This is for the triangle
	glGenVertexArrays(1, &VAOID);
	glBindVertexArray(VAOID);

	GLuint vL = glGetAttribLocation(Shader, "InVertex");
	GLuint nL = glGetAttribLocation(Shader, "InColor");
	GLuint mL = glGetAttribLocation(Shader, "m");

	//Bind the VBO and setup pointers for the VAO
	glBindBuffer(GL_ARRAY_BUFFER, VBOID);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex_VC), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TVertex_VC), BUFFER_OFFSET(sizeof(float)* 3));
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, GL_FALSE, BUFFER_OFFSET(sizeof(float)* 6));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	
	//Bind the IBO for the VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOID);

	//Just testing
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void display()
{
	glm::mat4 V;
	//glm::vec3 camPos;
	
	switch (model)
	{
	case CUBE:
		readin("cube.in");
		V = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, -2.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case COW:
		readin("cow_up.in");
		V = glm::lookAt(glm::vec3((x_min + x_max) / 2, (y_min + y_max) / 2, 1000 + (z_min + z_max) / 2),
			glm::vec3((x_min + x_max) / 2, (y_min + y_max) / 2, (z_min + z_max) / 2),
			glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case PHONE:
		readin("phone.in");
		V = glm::lookAt(glm::vec3((x_min + x_max) / 2 , (y_min + y_max) / 2, 1000 + (z_min + z_max) / 2),
			glm::vec3((x_min + x_max) / 2, (y_min + y_max) / 2, (z_min + z_max) / 2),
			glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	}

	CreateGeometry();

	glm::mat4 P = glm::perspective(45.0f, 4.0f / 3.0f, 1.0f, 1000.0f);
	glm::mat4 MVP = P * V;

	//Clear all the buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	//Bind the shader that we want to use
	glUseProgram(Shader);
	//Setup all uniforms for your shader
	SetUniform(Shader, glm::vec3(1.0f), glm::mat4(1.0f), V, MVP, sLight, dLight);
	//Bind the VAO
	glBindVertexArray(VAOID);
	//At this point, we would bind textures but we aren't using textures in this example

	//Draw command
	//The first to last vertex is 0 to 3
	//6 indices will be used to render the 2 triangles. This make our quad.
	//The last parameter is the start address in the IBO => zero
	glDrawElements(GL_TRIANGLES, NumV, GL_UNSIGNED_SHORT, 0);

	TwDraw();

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glutSwapBuffers();
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	TwWindowSize(w, h);
}

void ExitFunction(int value)
{
	cout << "Exit called." << endl;

	glBindVertexArray(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glUseProgram(0);

	glDeleteBuffers(1, &VBOID);
	glDeleteBuffers(1, &IBOID);
	glDeleteVertexArrays(1, &VAOID);

	glDetachShader(Shader, VertShader);
	glDetachShader(Shader, FragShader);
	glDeleteShader(VertShader);
	glDeleteShader(FragShader);
	glDeleteProgram(Shader);
}

int main(int argc, char* argv[])
{
	int i;
	int NumberOfExtensions;
	int OpenGLVersion[2];

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	//We want to make a GL 3.3 context
	glutInitContextVersion(3, 3);
	glutInitContextFlags(GLUT_CORE_PROFILE);
	glutInitWindowPosition(100, 50);
	glutInitWindowSize(800, 600);
	__glutCreateWindowWithExit("GL 3.3 Test", ExitFunction);

	//Currently, GLEW uses glGetString(GL_EXTENSIONS) which is not legal code
	//in GL 3.3, therefore GLEW would fail if we don't set this to TRUE.
	//GLEW will avoid looking for extensions and will just get function pointers for all GL functions.
	glewExperimental = TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		//Problem: glewInit failed, something is seriously wrong.
		cout << "glewInit failed, aborting." << endl;
		exit(1);
	}

	InitGLStates();
	InitializeUI();

	LoadShader("Shader1.vert", "Shader1.frag", false, false, true, Shader, VertShader, FragShader);
	
	
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);

	glutMainLoop();
	TwTerminate();
	return 0;
}

void TW_CALL Reset(void *clientDate){
	dLight.position = { 0.0f, 0.0f, 0.0f, 1.0f };
}