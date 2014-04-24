/* Written by Jeffrey Chastine, 2012 */
/* Lazily adapted by Mark Elsinger, 2014 */
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <string>
#include <fstream>
//#include <vector>
#include <random>
#include <ctime>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Mesh.h"

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
typedef unsigned char Uint8; //close enough

int windowHeight = 600;
int windowWidth = 800;
GLuint shaderProgramID;
GLuint vao[3];
GLuint vbo[3];
GLuint positionID, texcoordID, textures[3];
int elem_count[3];
//GLuint * indexArray = NULL;
GLfloat projection[4][4];
GLfloat * projection_p = (GLfloat *)projection;
//int ilen = 0;
float ball_vx = 0.0f, ball_vy = 0.0f, ball_vz = 0;
float ball_rx = 0.0f, ball_ry = 0.0f, ball_rz = 0.0f;
float ball_ay = -0.001f;
float opp_speed = 0.05f;
Mesh * player;
Mesh * opponent;
Mesh * ball;
Mesh * court;
int p_score = 0;
int o_score = 0;
//std::vector<Mesh *> static_objects, moving_objects, wall_objects; //quite ironic really...
//game variables


#pragma region SHADER_FUNCTIONS
static char* readFile(const char* filename) {
	/*// Open the file
	FILE* fp = fopen (filename, "r");
	// Move the file pointer to the end of the file and determing the length
	fseek(fp, 0, SEEK_END);
	long file_length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* contents = new char[file_length+1];*/

	//custom
	std::ifstream in(filename);
	// Move the file pointer to the end of the file and determing the length
	in.seekg(0, std::ios::end);
	long file_length = (long)in.tellg();
	in.clear();
	in.seekg(0, std::ios::beg);
	char* contents = new char[file_length + 1];

	// zero out memory
	for (int i = 0; i < file_length + 1; i++) {
		contents[i] = 0;
	}
	// Here's the actual read
	//fread (contents, 1, file_length, fp);
	in.read(contents, file_length);
	// This is how you denote the end of a string in C
	contents[file_length + 1] = '\0';


	//if (!in.good()) { printf("bad file"); } //debug line
	//fclose(fp);
	in.close();
	return contents;
}

bool compiledStatus(GLint shaderID){
	GLint compiled = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compiled);
	if (compiled) {
		return true;
	}
	else {
		GLint logLength;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
		char* msgBuffer = new char[logLength];
		glGetShaderInfoLog(shaderID, logLength, NULL, msgBuffer);
		printf("%s\n", msgBuffer);
		delete (msgBuffer);
		return false;
	}
}

GLuint makeVertexShader(const char* shaderSource) {
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderID, 1, (const GLchar**)&shaderSource, NULL);
	glCompileShader(vertexShaderID);
	bool compiledCorrectly = compiledStatus(vertexShaderID);
	if (compiledCorrectly) {
		return vertexShaderID;
	}
	return -1;
}

GLuint makeFragmentShader(const char* shaderSource) {
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderID, 1, (const GLchar**)&shaderSource, NULL);
	glCompileShader(fragmentShaderID);
	bool compiledCorrectly = compiledStatus(fragmentShaderID);
	if (compiledCorrectly) {
		return fragmentShaderID;
	}
	return -1;
}

GLuint makeShaderProgram(GLuint vertexShaderID, GLuint fragmentShaderID) {
	GLuint shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShaderID);
	glAttachShader(shaderID, fragmentShaderID);
	glLinkProgram(shaderID);
	return shaderID;
}
#pragma endregion SHADER_FUNCTIONS

#pragma region HELPER_FUNCTIONS


//tris only for now, restricted practical use. also not really for obj files...
void parseFlatObjFile(char * filename, GLfloat ** vertices, int * vlength, GLuint ** indices, int * ilength)
{
	GLfloat * verts = *vertices; //this is necessary for some reason...
	GLuint * inds = *indices;    //just using the arguments didn't work...

	std::ifstream in(filename);
	if (!in.good())
	{

		printf("bad file: %s\n", filename);
		system("PAUSE");
		exit(-1);
	}
	const int MAX_LINE_LENGTH = 80;
	char s[MAX_LINE_LENGTH + 1];
	int vtex_count = 0;
	int indx_count = 0;
	do
	{
		in.getline(s, MAX_LINE_LENGTH);
		//printf("%s\n", s); //debug
		if (s[0] == 'v')
		{
			vtex_count = vtex_count + 3;
		}
		if (s[0] == 'f')
		{
			indx_count = indx_count + 3;
		}
	} while (!in.eof());

	in.clear();
	in.seekg(0, std::ios::beg);

	*vlength = vtex_count;
	*ilength = indx_count;
	//gonna assume 3 per index and vertex!
	//*vertices = new GLfloat[vtex_count];
	//*indices = new GLuint[indx_count];
	verts = new GLfloat[vtex_count];
	inds = new GLuint[indx_count];

	int iv = 0;
	int ii = 0;
	char * tmp = NULL;
	char * nxt_tmp = NULL;
	do
	{
		in.getline(s, MAX_LINE_LENGTH);
		//printf("%s\n", s);
		if (s[0] == 'v')
		{
			tmp = strtok_s(&s[1], " ", &nxt_tmp);
			for (int i = 0; i < 3 && iv < vtex_count; ++i)
			{
				verts[iv] = (GLfloat)atof(tmp);
				tmp = strtok_s(NULL, " ", &nxt_tmp);
				++iv;
			}
		}
		else if (s[0] == 'f')
		{
			tmp = strtok_s(&s[1], " ", &nxt_tmp);
			for (int i = 0; i < 3 && ii < indx_count; ++i)
			{
				inds[ii] = (GLuint)atoi(tmp);
				tmp = strtok_s(NULL, " ", &nxt_tmp);
				++ii;
			}
		}

	} while (iv <= vtex_count && ii <= indx_count && !in.eof());

	*vertices = verts;
	*indices = inds;

	//should be good
	//printf("vcount: %d\nicount: %d\n", vtex_count, indx_count); //debug
	/*
	for (int i = 0; i < vtex_count; ++i)
	{
	printf("%f\n", verts[i]);
	}
	system("PAUSE");
	for (int i = 0; i < indx_count; ++i)
	{
	printf("%d\n", inds[i]);
	}
	*/
}

//pretty much same as above but provides texture mapping!
void parseUVObjFile(char * filename, GLfloat ** vertices, int * vlength, /*GLuint ** indices, int * ilength,*/ GLfloat ** texcoords, int * tlength, int * count)
{
	GLfloat * verts; //this is necessary for some reason...
	GLfloat * texs;

	GLuint ** inds;   //gotta store multiple index info now...
	

	std::ifstream in(filename);
	if (!in.good())
	{

		printf("bad file: %s\n", filename);
		system("PAUSE");
		exit(-1);
	}
	const int MAX_LINE_LENGTH = 80;
	char s[MAX_LINE_LENGTH + 1];
	int vtex_count = 0;
	int indx_count = 0;
	int texs_count = 0;
	do
	{
		in.getline(s, MAX_LINE_LENGTH);
		//printf("%s\n", s); //debug
		if (s[0] == 'v' && s[1] != 't')
		{
			vtex_count = vtex_count + 3;
		}
		if (s[0] == 'f')
		{
			indx_count = indx_count + 3;
		}
		if (s[0] == 'v' && s[1] == 't')
		{
			texs_count = texs_count + 2;
		}
	} while (!in.eof());

	in.clear();
	in.seekg(0, std::ios::beg);

	//*vlength = vtex_count;
	//*tlength = texs_count;
	int ilength = indx_count;
	//gonna assume 3 per index and vertex!
	//*vertices = new GLfloat[vtex_count];
	//*indices = new GLuint[indx_count];
	verts = new GLfloat[vtex_count];
	texs = new GLfloat[texs_count];
	printf("OBJ vtex count: %d\nOBJ tex count: %d\n", vtex_count, texs_count);
	inds = new GLuint*[2]; //stores the vert and tex order.
	inds[0] = new GLuint[indx_count];
	inds[1] = new GLuint[indx_count];


	int iv = 0;
	int ii = 0;
	int it = 0;
	char * tmp = NULL;
	char * nxt_tmp = NULL;
	do
	{
		in.getline(s, MAX_LINE_LENGTH);
		//printf("%s\n", s);
		if (s[0] == 'v' && s[1] != 't')
		{
			tmp = strtok_s(&s[1], " ", &nxt_tmp);
			for (int i = 0; i < 3 && iv < vtex_count; ++i)
			{
				verts[iv] = (GLfloat)atof(tmp);
				tmp = strtok_s(NULL, " ", &nxt_tmp);
				++iv;
			}
		}
		else if (s[0] == 'f')
		{
			tmp = strtok_s(&s[1], " ", &nxt_tmp);
			for (int i = 0; i < 3 && ii < indx_count; ++i)
			{
				//printf("%s\n", tmp);
				unsigned int slashind = 0;
				for (; slashind < strlen(tmp); ++slashind)
				{
					if (tmp[slashind] == '/')
					{
						tmp[slashind] = '\0';	//find the slash, replace with terminator
						break;
					}
				}

				inds[0][ii] = (GLuint)atoi(tmp); //converts until the terminator, the whole vertex part
				inds[1][ii] = (GLuint)atoi(&tmp[slashind + 1]); //converts after terminator to final (hopefully), the tex part.
				//printf("%s\n", tmp);
				//printf("%s\n", &tmp[slashind +1]);
				tmp = strtok_s(NULL, " ", &nxt_tmp); //get next
				++ii;
			}
		}
		else if (s[0] == 'v' && s[1] == 't')
		{
			tmp = strtok_s(&s[2], " ", &nxt_tmp);
			for (int i = 0; i < 2 && it < texs_count; ++i) //assume only u&v coords
			{
				texs[it] = (GLfloat)atof(tmp);
				tmp = strtok_s(NULL, " ", &nxt_tmp);
				++it;
			}
		}

	} while (iv <= vtex_count && ii <= indx_count && it <= texs_count && !in.eof());

	//reorder verts and tex coordinates to remove indexes. this is necessary for proper texture mapping.
	GLfloat * final_verts = new GLfloat[indx_count * 3];
	GLfloat * final_texs = new GLfloat[indx_count * 2];
	iv = 0;
	it = 0;
	for (int i = 0; i < indx_count; i = ++i)
	{
		int vert_ind = (inds[0][i] - 1) * 3;		//get vert index, minus one because obj format
		int tex_ind = (inds[1][i] - 1) * 2;			//then get texture index
		final_verts[iv] = verts[vert_ind];
		final_verts[iv + 1] = verts[vert_ind + 1];	//get all the index data
		final_verts[iv + 2] = verts[vert_ind + 2];
		final_texs[it] = texs[tex_ind];				
		final_texs[it + 1] = texs[tex_ind + 1];		//and all the texture data

		iv = iv + 3;
		it = it + 2;
	}

	//finish up, set return agruments and delete unused arrays.
	*vertices = final_verts;
	//*indices = inds;
	*texcoords = final_texs;
	*vlength = indx_count * 3;
	*tlength = indx_count * 2;
	*count = indx_count;

	free(verts);
	free(texs);
	free(inds[0]);
	free(inds[1]);
	free(inds);

	//should be good
	printf("vcount: %d\nicount: %d\n", vtex_count, indx_count); //debug
	/*
	for (int i = 0; i < vtex_count; ++i)
	{
	printf("%f\n", verts[i]);
	}
	system("PAUSE");
	for (int i = 0; i < indx_count; ++i)
	{
	printf("%d\n", inds[i]);
	}
	system("PAUSE");
	for (int i = 0; i < texs_count; ++i)
	{
	printf("%f\n", texs[i]);
	}
	system("PAUSE"); */
}

//this is for 4x4, hard coded.
void createPerspectiveMatrix4(float fov, float aspect, float _near, float _far, GLfloat out_m[4][4])
{

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	float angle = (fov / 180.0f) * (float)M_PI;
	float f = 1.0f / tan(angle * 0.5f);

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = f / aspect;
	out_m[1][1] = f;
	out_m[2][2] = (_far + _near) / (_near - _far);
	out_m[2][3] = -1.0f;
	out_m[3][2] = (2.0f * _far * _near) / (_near - _far);


	/*
	out_m[0][0] = f / aspect;
	out_m[1][1] = f;
	out_m[2][2] = (_far + _near) / (_near - _far);
	out_m[2][3] = (2.0f * _far * _near) / (_near - _far);
	out_m[3][2] = 1.0f;
	*/

	//return m;
}

void createIdentityMatrix4(GLfloat out_m[4][4])
{

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = 1.0f;
	out_m[1][1] = 1.0f;
	out_m[2][2] = 1.0f;
	out_m[3][3] = 1.0f;

	//return m;
}

void createScaleMatrix4(float sx, float sy, float sz, GLfloat out_m[4][4])
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = sx;
	out_m[1][1] = sy;
	out_m[2][2] = sz;
	out_m[3][3] = 1.0f;
}

void createTranslationMatrix4(float x, float y, float z, GLfloat out_m[4][4])
{

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = 1.0f;
	out_m[1][1] = 1.0f;
	out_m[2][2] = 1.0f;
	out_m[3][0] = x;
	out_m[3][1] = y;
	out_m[3][2] = z;
	out_m[3][3] = 1.0f;

	//return m;
}

void createRotationMatrixX4(float roll, GLfloat out_m[4][4])
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = 1.0f;
	out_m[1][1] = cos(roll);
	out_m[1][2] = -sin(roll);
	out_m[2][1] = sin(roll);
	out_m[2][2] = cos(roll);
	out_m[3][3] = 1.0f;

}

void createRotationMatrixY4(float yaw, GLfloat out_m[4][4])
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = cos(yaw);
	out_m[0][2] = sin(yaw);
	out_m[1][1] = 1.0f;
	out_m[2][0] = -sin(yaw);
	out_m[2][2] = cos(yaw);
	out_m[3][3] = 1.0f;

}

void createRotationMatrixZ4(float pitch, GLfloat out_m[4][4])
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			out_m[i][j] = 0.0f;
		}
	}

	/* Note, matrices are accessed like 2D arrays in C.
	They are column major, i.e m[y][x] */

	out_m[0][0] = cos(pitch);
	out_m[0][1] = -sin(pitch);
	out_m[1][0] = sin(pitch);
	out_m[1][1] = cos(pitch);
	out_m[2][2] = 1.0f;
	out_m[3][3] = 1.0f;

}

//kinda need windows for this bit. easily ported to linux though, just need libraries...
//also nabbed this from http://www.cplusplus.com/articles/GwvU7k9E/, thanks Fredbill =]
int loadBMP(const char* location, GLuint *texture)
{
	Uint8* datBuff[2] = { nullptr, nullptr }; // Header buffers

	Uint8* pixels = nullptr; // Pixels

	BITMAPFILEHEADER* bmpHeader = nullptr; // Header
	BITMAPINFOHEADER* bmpInfo = nullptr; // Info 

	// The file... We open it with it's constructor
	std::ifstream file(location, std::ios::binary);
	if (!file)
	{
		printf("Failure to open bitmap file.\n");

		return 1;
	}

	// Allocate byte memory that will hold the two headers
	datBuff[0] = new Uint8[sizeof(BITMAPFILEHEADER)];
	datBuff[1] = new Uint8[sizeof(BITMAPINFOHEADER)];

	file.read((char*)datBuff[0], sizeof(BITMAPFILEHEADER));
	file.read((char*)datBuff[1], sizeof(BITMAPINFOHEADER));

	// Construct the values from the buffers
	bmpHeader = (BITMAPFILEHEADER*)datBuff[0];
	bmpInfo = (BITMAPINFOHEADER*)datBuff[1];

	// Check if the file is an actual BMP file
	if (bmpHeader->bfType != 0x4D42)
	{
		printf("File \"%s\" isn't a bitmap file\n", location);
		return 2;
	}

	// First allocate pixel memory
	pixels = new Uint8[bmpInfo->biSizeImage];

	// Go to where image data starts, then read in image data
	file.seekg(bmpHeader->bfOffBits);
	file.read((char*)pixels, bmpInfo->biSizeImage);

	// We're almost done. We have our image loaded, however it's not in the right format.
	// .bmp files store image data in the BGR format, and we have to convert it to RGB.
	// Since we have the value in bytes, this shouldn't be to hard to accomplish
	Uint8 tmpRGB = 0; // Swap buffer
	for (unsigned long i = 0; i < bmpInfo->biSizeImage; i += 3)
	{
		tmpRGB = pixels[i];
		pixels[i] = pixels[i + 2];
		pixels[i + 2] = tmpRGB;
	}

	// Set width and height to the values loaded from the file
	GLuint w = bmpInfo->biWidth;
	GLuint h = bmpInfo->biHeight;

	/*******************GENERATING TEXTURES*******************/

	//enable that texturing?
	glEnable(GL_TEXTURE_2D);
	//glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, texture);             // Generate a texture
	glBindTexture(GL_TEXTURE_2D, *texture); // Bind that texture temporarily

	GLint mode = GL_RGB;                   // Set the mode

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Create the texture. We get the offsets from the image, then we use it with the image's
	// pixel data to create it.
	glTexImage2D(GL_TEXTURE_2D, 0, mode, w, h, 0, mode, GL_UNSIGNED_BYTE, pixels);

	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, NULL);

	// Output a successful message
	printf("Texture \"%s\" successfully loaded.\n", location);

	// Delete the two buffers.
	delete[] datBuff[0];
	delete[] datBuff[1];
	delete[] pixels;

	return 0; // Return success code 
}

#pragma endregion HELPER_FUNCTIONS


// Any time the window is resized, this function gets called.  It's setup to the
// "glutReshapeFunc" in main.
void changeViewport(int w, int h){
	glViewport(0, 0, w, h);
	windowHeight = h;
	windowWidth = w;
}

void renderMesh(Mesh m)
{

	//GLfloat model[4][4], view[4][4], projection[4][4];
	GLfloat modelT[4][4], modelX[4][4], modelY[4][4], modelZ[4][4], modelS[4][4], viewp[4][4], viewr[4][4];
	/*
	createTranslationMatrix4(m.getX(), m.getY(), m.getZ(), modelT);
	createRotationMatrixX4(m.getRoll(), modelX);
	createRotationMatrixY4(m.getPitch(), modelY);
	createRotationMatrixZ4(m.getYaw(), modelZ);
	createScaleMatrix4(m.getScaleX(), m.getScaleY(), m.getScaleY(), modelS);
	*/
	createTranslationMatrix4(m.x, m.y, m.z, modelT);
	createRotationMatrixX4(m.roll, modelX);
	createRotationMatrixY4(m.pitch, modelY);
	createRotationMatrixZ4(m.yaw, modelZ);
	createScaleMatrix4(m.scaleX, m.scaleY, m.scaleZ, modelS);

	createTranslationMatrix4(-(player->x), -15.0f, -(player->z + 20.0f), viewp);
	createRotationMatrixX4(-0.5f, viewr);

	GLfloat * modelT_p = (GLfloat *)modelT;
	GLfloat * modelX_p = (GLfloat *)modelX;
	GLfloat * modelY_p = (GLfloat *)modelY;
	GLfloat * modelZ_p = (GLfloat *)modelZ;
	GLfloat * modelS_p = (GLfloat *)modelS;
	GLfloat * viewp_p = (GLfloat *)viewp;
	GLfloat * viewr_p = (GLfloat *)viewr;




	/*for (int i = 0; i < 4; ++i)
	{
	for (int j = 0; j < 4; ++j)
	{
	printf("%f ", view[i][j]);
	}
	printf("\n");
	}*/

	GLint tempLoc = glGetUniformLocation(shaderProgramID, "s_mMT");//Matrix that handle the transformations
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, modelT_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mMX");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, modelX_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mMY");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, modelY_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mMZ");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, modelZ_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mMS");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, modelS_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mVp");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, viewp_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mVr");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, viewr_p);

	tempLoc = glGetUniformLocation(shaderProgramID, "s_mP");
	glUniformMatrix4fv(tempLoc, 1, GL_FALSE, projection_p);

	//tempLoc = glGetUniformLocation(shaderProgramID, "color");
	//glUniform4f(tempLoc, m.red, m.blue, m.green, 1.0f);

	//glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(vao[m.objectIndex]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[m.objectIndex]);
	glBindTexture(GL_TEXTURE_2D, textures[m.objectIndex]);
	//glDrawElements(GL_TRIANGLES, ilen, GL_UNSIGNED_INT, NULL);
	glDrawArrays(GL_TRIANGLES, 0, elem_count[m.objectIndex]);
	glBindTexture(GL_TEXTURE_2D, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	glBindVertexArray(NULL);
}


void gameInit()
{
	srand((unsigned int)time(NULL));
	player = new Mesh("player", 1); //create the player
	player->translate(0.0f, 1.0f, 10.0f);				//move them back a bit.
	player->pitch = -1.57079f;		//rotate to face the right direction.
	//player->scale(0.5f, 0.5f, 0.5f);

	opponent = new Mesh("opponent", 1); //create opponent and move them forward a bit.
	opponent->translate(0.0f, 1.0f, -10.0f);
	opponent->pitch = 1.57079f;
	//opponent->scale(0.5f, 0.5f, 0.5f);

	court = new Mesh("floor", 2); //create court, needs to be bigger...
	//court->y = 10.0f;
	court->scale(20.0f, 1.0f, 20.0f);
	court->roll = 3.14159f;

	ball = new Mesh("ball", 0); //place ball above player
	ball->translate(0.0f, 10.0f, 10.0f);
	
	printf("Game init'd!\n");
}

void endGame()
{
	printf("Game Over\n");
	//printf("Score: %d\n", score);
	system("pause");
	free(player);
	free(opponent);
	free(court);
	free(ball);
	exit(1);
}

void reset()
{
	ball->x = 0.0f;
	ball->y = 10.0f;
	ball->z = 10.0f;
	ball->roll = 0.0f;
	ball->pitch = 0.0f;
	ball->yaw = 0.0f;
	ball_rx = 0.0f;
	ball_ry = 0.0f;
	ball_rz = 0.0f;
	ball_vx = 0.0f;
	ball_vy = 0.0f;
	ball_vz = 0.0f;
}

//updates the world/ball and checks collisions, game logic.
//be sure to call initGame.
void updateWorld()
{
	bool collided = false;

	ball_vy += ball_ay;
	ball->y += ball_vy;
	ball->x += ball_vx;
	ball->z += ball_vz;
	ball->roll += ball_rx;
	ball->pitch += ball_ry;
	ball->yaw += ball_rz;

	if (ball->collidesWith(*player) && ball_vy < 0.0f)
	{
		collided = true;
		float oldvx = ball_vx;
		float oldvz = ball_vz;
		ball_vx = 0.05f * (ball->x - player->x);
		ball_vz = 0.05f * (ball->z - player->z);
		ball_vy = -(ball_vy);
	}
	else if (ball->collidesWith(*opponent) && ball_vy < 0.0f)
	{
		collided = true;
		float oldvx = ball_vx;
		float oldvz = ball_vz;
		ball_vx = 0.05f * (ball->x - opponent->x);
		ball_vz = 0.05f * (ball->z - opponent->z);
		ball_vy = -(ball_vy);
	}
	else if (ball->y < 1.0f) //hit ground, not saved
	{
		if (ball->z > 0.0f)	//on player's side
		{
			if (ball->x < 10.0f
				&& ball->x > -10.0f
				&& ball->z < 20.0f)
			{
				o_score += 1;	//in the scoring zone
				if (o_score >= 5)
				{
					printf("Opponent Won! Score: %d:%d\n", p_score, o_score);
				}
				else
				{
					printf("Opponent Scored! Score: %d:%d\n", p_score, o_score);
				}
				reset();
			}
			else
			{
				printf("Out of bounds!\n");
				reset();
			}
		}
		else //on opponents side
		{
			if (ball->x < 10.0f 
				&& ball->x > -10.0f
				&& ball->z > -20.0f)
			{
				p_score += 1;	//in the scoring zone
				if (p_score >= 5)
				{
					printf("Player Won! Score: %d:%d\n", p_score, o_score);
				}
				else
				{
					printf("Player Scored! Score: %d:%d\n", p_score, o_score);
				}
				reset();
			}
			else
			{
				printf("Out of bounds!\n");
				reset();
			}
		}
	}
	if (collided)
	{
		ball_rx = 0.001f * (float)(rand() % 100);
		ball_ry = 0.001f * (float)(rand() % 100);
		ball_rz = 0.001f * (float)(rand() % 100);
	}
}

void updateAI()
{
	float targetx = 1.1f * ball->x;
	float targetz = 1.1f * ball->z;
	if (targetx - opponent->x > opp_speed)
	{
		opponent->x += opp_speed;
	}
	else if (targetx - opponent->x < -opp_speed)
	{
		opponent->x -= opp_speed;
	}
	else
	{
		opponent->x = targetx;
	}

	if (targetz - opponent->z > opp_speed)
	{
		opponent->z += opp_speed;
	}
	else if (targetz - opponent->z < -opp_speed)
	{
		opponent->z -= opp_speed;
	}
	else
	{
		opponent->z = targetz;
	}

	if (opponent->x > 20.0f)
	{
		opponent->x = 20.0f;
	}
	else if (opponent->x < -20.0f)
	{
		opponent->x = -20.0f;
	}

	if (opponent->z > 0.0f)
	{
		opponent->z = 0.0f;
	}
	else if (opponent->z < -20.0f)
	{
		opponent->z = -20.0f;
	}
}

// Here is the function that gets called each time the window needs to be redrawn.
// It is the "paint" method for our program, and is set up from the glutDisplayFunc in main
void render() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/*for each(Mesh* m in wall_objects)
	{
		renderMesh(*m);
	}

	for each(Mesh* m in static_objects)
	{
		renderMesh(*m);
	}*/

	renderMesh(*player);
	renderMesh(*opponent);
	renderMesh(*court);
	renderMesh(*ball);

	glutSwapBuffers();
	//currentYaw += 0.01f; //debug
}

//realy the update func
void update(int val)
{
	//currentYaw += dYaw; //debug
	//currentRoll += dRoll;
	//currentPitch += dPitch;

	if (o_score < 5 && p_score < 5)
	{
		updateWorld();
		updateAI();
	}

	glutPostRedisplay();
	glutTimerFunc(17, update, 1);
}

void mouse(int button, int state, int x, int y)
{
}

void motion(int x, int y)
{
	player->x = (40.0f / (windowWidth))*(x - (0.5f * (windowWidth)));
	player->z = (20.0f / (windowHeight))*(y - (0.5f * (windowHeight))) + 10.0f;
}

void keyboard(unsigned char key, int x, int y)
{

} 


int main(int argc, char** argv) {
	// Standard stuff...
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("BounceBall");
	glutReshapeFunc(changeViewport);
	glutDisplayFunc(render);
	glutTimerFunc(17, update, 1);
	//glutMouseFunc(mouse);
	glutPassiveMotionFunc(motion);
	//glutKeyboardFunc(keyboard);
	glewInit();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	//set up matrices
	createPerspectiveMatrix4(45.0f, 1.6f, 1.0f, 75.0f, projection);
	//CreateIdentityMatrix4(projection);



	// Make a shader
	char* vertexShaderSourceCode = readFile("vertexShader.glsl");
	char* fragmentShaderSourceCode = readFile("fragmentShader.glsl");

	//printf("vtex shader: %s\n", vertexShaderSourceCode); // debug stuff
	//printf("fment shader: %s\n", fragmentShaderSourceCode);

	GLuint vertShaderID = makeVertexShader(vertexShaderSourceCode);
	GLuint fragShaderID = makeFragmentShader(fragmentShaderSourceCode);
	shaderProgramID = makeShaderProgram(vertShaderID, fragShaderID);

	printf("vertShaderID is %d\n", vertShaderID);
	printf("fragShaderID is %d\n", fragShaderID);
	printf("shaderProgramID is %d\n", shaderProgramID);

	// Find the position of the variables in the shader
	positionID = glGetAttribLocation(shaderProgramID, "s_vPosition");
	texcoordID = glGetAttribLocation(shaderProgramID, "s_vTexcoord");
	printf("s_vPosition's ID is %d\n", positionID);
	printf("s_vTexcoord's ID is %d\n", texcoordID);

	GLuint texID = glGetUniformLocation(shaderProgramID, "s_fTexture");
	printf("s_fTexture location: %d\n", texID);

	glUniform1i(texID, 0); //point dat uniform to the texture.

	glUseProgram(shaderProgramID);

	// Create the vaos & vbos
	glGenVertexArrays(3, vao);
	glGenBuffers(3, vbo);

	//load in the stuff for the beachball
	glBindVertexArray(vao[0]);

	GLfloat * vertices = NULL, *texcoord = NULL;
	//GLuint * indices = NULL;
	int vlen = 0, tlen = 0;
	//int ilen = 0;


	//parseFlatObjFile("plane.obj", &vertices, &vlen, &indexArray, &ilen);
	parseUVObjFile("sphere2.obj", &vertices, &vlen, /*&indexArray, &ilen,*/ &texcoord, &tlen, &elem_count[0]);


	printf("vlen: %d\n", vlen);
	printf("tlen: %d\n", tlen);
	//printf("ilen: %d\n", ilen);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	// Create the buffer, but don't load anything yet
	glBufferData(GL_ARRAY_BUFFER, (vlen + tlen)*(sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	// Load the vertex points
	glBufferSubData(GL_ARRAY_BUFFER, 0, vlen*sizeof(GLfloat), vertices);
	// Load the colors right after that
	glBufferSubData(GL_ARRAY_BUFFER, vlen*sizeof(GLfloat), tlen*sizeof(GLfloat), texcoord);


	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(texcoordID, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vlen*sizeof(GLfloat)));
	//glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionID);
	glEnableVertexAttribArray(texcoordID);

	//set up some stuff, maybe
	//gluPerspective(45.0f, 1.6f, 0.1f, 10.0f);
	glActiveTexture(GL_TEXTURE0);
	loadBMP("beachball.bmp", &textures[0]); //remember, volleyball is texture #0!

	glBindVertexArray(NULL);


	//load in the stuff for the bouncer thingy
	glBindVertexArray(vao[1]);

	*vertices = NULL;
	*texcoord = NULL;
	//GLuint * indices = NULL;
	vlen = 0;
	tlen = 0;
	//int ilen = 0;

	parseUVObjFile("bouncer.obj", &vertices, &vlen, /*&indexArray, &ilen,*/ &texcoord, &tlen, &elem_count[1]);

	printf("vlen: %d\n", vlen);
	printf("tlen: %d\n", tlen);
	//printf("ilen: %d\n", ilen);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	// Create the buffer, but don't load anything yet
	glBufferData(GL_ARRAY_BUFFER, (vlen + tlen)*(sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	// Load the vertex points
	glBufferSubData(GL_ARRAY_BUFFER, 0, vlen*sizeof(GLfloat), vertices);
	// Load the colors right after that
	glBufferSubData(GL_ARRAY_BUFFER, vlen*sizeof(GLfloat), tlen*sizeof(GLfloat), texcoord);


	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(texcoordID, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vlen*sizeof(GLfloat)));
	//glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionID);
	glEnableVertexAttribArray(texcoordID);

	//set up some stuff, maybe
	//gluPerspective(45.0f, 1.6f, 0.1f, 10.0f);
	glActiveTexture(GL_TEXTURE0);
	loadBMP("bouncerSkin.bmp", &textures[1]);

	glBindVertexArray(NULL);


	//load in the stuff for the floor
	glBindVertexArray(vao[2]);

	*vertices = NULL;
	*texcoord = NULL;
	//GLuint * indices = NULL;
	vlen = 0;
	tlen = 0;
	//int ilen = 0;

	parseUVObjFile("floor.obj", &vertices, &vlen, /*&indexArray, &ilen,*/ &texcoord, &tlen, &elem_count[2]);

	printf("vlen: %d\n", vlen);
	printf("tlen: %d\n", tlen);
	//printf("ilen: %d\n", ilen);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	// Create the buffer, but don't load anything yet
	glBufferData(GL_ARRAY_BUFFER, (vlen + tlen)*(sizeof(GLfloat)), NULL, GL_STATIC_DRAW);
	// Load the vertex points
	glBufferSubData(GL_ARRAY_BUFFER, 0, vlen*sizeof(GLfloat), vertices);
	// Load the colors right after that
	glBufferSubData(GL_ARRAY_BUFFER, vlen*sizeof(GLfloat), tlen*sizeof(GLfloat), texcoord);


	glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(texcoordID, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(vlen*sizeof(GLfloat)));
	//glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(positionID);
	glEnableVertexAttribArray(texcoordID);

	//set up some stuff, maybe
	//gluPerspective(45.0f, 1.6f, 0.1f, 10.0f);
	glActiveTexture(GL_TEXTURE0);
	loadBMP("court.bmp", &textures[2]);

	glBindVertexArray(NULL);

	//load up game stuff
	gameInit();

	glutMainLoop();

	return 0;
}