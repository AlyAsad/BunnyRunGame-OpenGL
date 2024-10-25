#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>   // The GL Header File
#include <GL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[4];
GLint gIntensityLoc;
GLuint gSamplerLocation;
float gIntensity = 100;
int gWidth = 1000, gHeight = 750;


// ALY: my own definitions

int totalObjects = 4;

// sky
GLuint skyProgram;
GLuint skySampler, skyTexture;

//for score
float score = 0;
bool rHeld = false;

// overall speed counter
double speed = 1.0;

// if dead or not
bool dead = false;

//bunny stats
float hop = 0.0;
float hoppingSpeed = 1;
float laying = 0;
bool aHeld = false;
bool dHeld = false;
float mov = 0;
float movSpeed = 1;
float spin = 0;
float spinSpeed = 1;

// road
bool odd = true;
float roll = 0; 
float rollingSpeed = 1;
vector<int> rolled(30, 0);


// cubes
int currYellow = rand() % 3;
float cubeDisp = 0;
float cubeSpeed = 1;
int collided = -1;


// ALY definitions over

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

vector<vector<Vertex>> gVertices(totalObjects);
vector<vector<Texture>> gTextures(totalObjects);
vector<vector<Normal>> gNormals(totalObjects);
vector<vector<Face>> gFaces(totalObjects);

vector<GLuint> gVertexAttribBuffer(totalObjects), gIndexBuffer(totalObjects);
GLuint gTextVBO;
GLint gInVertexLoc, gInNormalLoc;
vector<int> gVertexDataSizeInBytes(totalObjects), gNormalDataSizeInBytes(totalObjects);

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;


bool ParseObj(const string& fileName, int i)
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures[i].push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals[i].push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices[i].push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0]; 
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1]; 
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2]; 

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces[i].push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }


	assert(gVertices[i].size() == gNormals[i].size());

    return true;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();
    gProgram[2] = glCreateProgram();
    gProgram[3] = glCreateProgram();
    skyProgram = glCreateProgram();
    

    createVS(gProgram[0], "vert0.glsl");
    createFS(gProgram[0], "frag0.glsl");

    createVS(gProgram[1], "vert1.glsl");
    createFS(gProgram[1], "frag1.glsl");
    
    createVS(skyProgram, "skyV.glsl");
    createFS(skyProgram, "skyF.glsl"); //sky texture

    createVS(gProgram[2], "vert_text.glsl");
    createFS(gProgram[2], "frag_text.glsl"); //rendering text
    
    createVS(gProgram[3], "vertCubes.glsl");
    createFS(gProgram[3], "fragCubes.glsl"); // cubes

    glBindAttribLocation(gProgram[0], 0, "inVertex");
    glBindAttribLocation(gProgram[0], 1, "inNormal");
    glBindAttribLocation(gProgram[1], 0, "inVertex");
    glBindAttribLocation(gProgram[1], 1, "inNormal");
    
    glBindAttribLocation(skyProgram, 0, "inVertex");
    glBindAttribLocation(skyProgram, 1, "TexCoord");
    
    glBindAttribLocation(gProgram[2], 1, "inNormal");
    glBindAttribLocation(gProgram[2], 2, "vertex");
    
    glBindAttribLocation(gProgram[3], 0, "inVertex");
    glBindAttribLocation(gProgram[3], 1, "inNormal");

    glLinkProgram(gProgram[0]);
    glLinkProgram(gProgram[1]);
    
    glLinkProgram(skyProgram);
    glLinkProgram(gProgram[2]);
    glLinkProgram(gProgram[3]);
    
    glUseProgram(gProgram[0]);

    gIntensityLoc = glGetUniformLocation(gProgram[0], "intensity");
    glUniform1f(gIntensityLoc, gIntensity);
    glUseProgram(gProgram[3]);
    gIntensityLoc = glGetUniformLocation(gProgram[3], "intensity");
    glUniform1f(gIntensityLoc, gIntensity);
    gSamplerLocation = glGetUniformLocation(skyProgram, "gSampler");
}


void initTextures()
{
	stbi_set_flip_vertically_on_load(1);
	glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
	glEnable(GL_TEXTURE_2D);
	
	glGenTextures(1, &skyTexture);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	int w, h, nrChannels;
	unsigned char *imgPix = stbi_load("sky.jpg", &w, &h, &nrChannels, 0);
	
 	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, imgPix);
	
	stbi_image_free(imgPix);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	
}

void initVBO()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    assert(glGetError() == GL_NONE);
	
    

	for (int j = 0; j < totalObjects; j++) {
	glGenBuffers(1, &(gVertexAttribBuffer[j]));
	glGenBuffers(1, &(gIndexBuffer[j]));
    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer[j]);
   	
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer[j]);
    gVertexDataSizeInBytes[j] = gVertices[j].size() * 3 * sizeof(GLfloat);
    gNormalDataSizeInBytes[j] = gNormals[j].size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = gFaces[j].size() * 3 * sizeof(GLuint);
    GLfloat* vertexData = new GLfloat [gVertices[j].size() * 3];
    GLfloat* normalData = new GLfloat [gNormals[j].size() * 3];
    GLuint* indexData = new GLuint [gFaces[j].size() * 3];

    float minX = 1e6, maxX = -1e6;
    float minY = 1e6, maxY = -1e6;
    float minZ = 1e6, maxZ = -1e6;
    
    for (int i = 0; i < gVertices[j].size(); ++i)
    {
        vertexData[3*i] = gVertices[j][i].x;
        vertexData[3*i+1] = gVertices[j][i].y;
        vertexData[3*i+2] = gVertices[j][i].z;

        minX = std::min(minX, gVertices[j][i].x);
        maxX = std::max(maxX, gVertices[j][i].x);
        minY = std::min(minY, gVertices[j][i].y);
        maxY = std::max(maxY, gVertices[j][i].y);
        minZ = std::min(minZ, gVertices[j][i].z);
        maxZ = std::max(maxZ, gVertices[j][i].z);
    }
	

    for (int i = 0; i < gNormals[j].size(); ++i)
    {
        normalData[3*i] = gNormals[j][i].x;
        normalData[3*i+1] = gNormals[j][i].y;
        normalData[3*i+2] = gNormals[j][i].z;
    }

    for (int i = 0; i < gFaces[j].size(); ++i)
    {
        indexData[3*i] = gFaces[j][i].vIndex[0];
        indexData[3*i+1] = gFaces[j][i].vIndex[1];
        indexData[3*i+2] = gFaces[j][i].vIndex[2];
    }


    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[j] + gNormalDataSizeInBytes[j], 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes[j], vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes[j], gNormalDataSizeInBytes[j], normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);
	
    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[j]));
	
	}
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint) face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init() 
{
	ParseObj("bunny.obj", 0);
	ParseObj("quad.obj", 1);
	ParseObj("cube.obj", 3);
	
    glEnable(GL_DEPTH_TEST);
    initShaders();
    initTextures();
    initFonts(gWidth, gHeight);
    initVBO();
}



void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}


void drawModel(int i)
{	
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer[i]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer[i]);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes[i]));

	glDrawElements(GL_TRIANGLES, gFaces[i].size() * 3, GL_UNSIGNED_INT, 0);
	
}


void renderSky() {

    glUseProgram(skyProgram);
    glUniform1i(glGetUniformLocation(skyProgram, "texturemap"), 0);
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, skyTexture);
	glUniform1i(gSamplerLocation, 0);
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer[1]);
	
	glm::mat4 T, S, modelMat, modelMatInv, perspMat;
	
	T = glm::translate(glm::mat4(1.f), glm::vec3(-0.5f, -0.5f, -70.f));
	S = glm::scale(glm::mat4(1.f), glm::vec3(58.33f, 58.33f, 1.f));
	modelMat = S * T;
	modelMatInv = glm::transpose(glm::inverse(modelMat));
    perspMat = glm::perspective(glm::radians(45.0f), 1.f, 1.0f, 100.0f);

    
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
	glUniformMatrix4fv(glGetUniformLocation(skyProgram, "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	
    glBindTexture(GL_TEXTURE_2D, 0);
    
}




void renderBunny() {
	
	glUseProgram(gProgram[0]);
    
    glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(0.f + mov, -1.5f + hop, -4.8f));
    glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(0.2f, 0.25f, 0.7f));
    glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(-90.f + spin), glm::vec3(0, 1, 0));
    glm::mat4 R2 = glm::rotate(glm::mat4(1.f), glm::radians(laying), glm::vec3(1, 0, 0));
    glm::mat4 modelMat = T * S * R * R2;
    glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
    glm::mat4 perspMat = glm::perspective(glm::radians(45.0f), 1.f, 1.0f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));

    drawModel(0);

}



void renderTiles() {

	glm::mat4 T, S, R, T2, S2, modelMat, modelMatInv, temp;
	glUseProgram(gProgram[1]);
    
    T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -2.f, -20.f));
    S = glm::scale(glm::mat4(1.f), glm::vec3(0.25f, 0.3f, 1.f));
    R = glm::rotate(glm::mat4(1.f), glm::radians(-90.f), glm::vec3(1, 0, 0));
    S2 = glm::scale(glm::mat4(1.f), glm::vec3(1.f, 3.f, 1.f));
    glm::mat4 perspMat = glm::perspective(glm::radians(18.0f), 1.f, 1.0f, 200.0f);
    temp = S * T * R;
    
    for (int i = 0; i < 30; i++) {
    	odd = !odd;
    for (int j = 0; j < 4; j++) {
    
    float t = (-20.f + 6*i) - roll*5 + (float)(180*rolled[i]);
    if (t < -20 && j == 0) {
   		rolled[i]++;
   		t += 180;
    }
    
    T2 = glm::translate(glm::mat4(1.f), glm::vec3(-3.f + 2*j, t, 0.f));
    modelMat = temp * T2 * S2;
    modelMatInv = glm::transpose(glm::inverse(modelMat));
    
    glUniform1i(glGetUniformLocation(gProgram[1], "isOdd"), odd);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[1], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[1], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[1], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));

    drawModel(1);
    
    odd = !odd;
    }
    }
    
}



void renderCheckpoints() {
	
	glm::mat4 T, S, R, modelMat, modelMatInv, perspMat, temp;
	glUseProgram(gProgram[3]);
    
    S = glm::scale(glm::mat4(1.f), glm::vec3(0.012f, 0.05f, 0.085f));
    R = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1, 0, 0));
    
    perspMat = glm::perspective(glm::radians(18.0f), 1.f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "perspectiveMat"), 1, GL_FALSE, glm::value_ptr(perspMat));
    glUniform1i(glGetUniformLocation(gProgram[3], "currYellow"), currYellow);
    
    // left cube
	T = glm::translate(glm::mat4(1.f), glm::vec3(-0.082f + (-7.f + cubeDisp)*0.0035, -0.1f, -7.f + cubeDisp));
    modelMat = T * R * S;
    modelMatInv = glm::transpose(glm::inverse(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniform1i(glGetUniformLocation(gProgram[3], "curr"), 0);
    
    if (collided != 0)
    	drawModel(3);
    
    // middle cube
	T = glm::translate(glm::mat4(1.f), glm::vec3(0.f, -0.1f, -7.f + cubeDisp));
    modelMat = T * R * S;
    modelMatInv = glm::transpose(glm::inverse(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniform1i(glGetUniformLocation(gProgram[3], "curr"), 1);
	
	if (collided != 1)
		drawModel(3);
	
	//right cube
	T = glm::translate(glm::mat4(1.f), glm::vec3(0.082f - (-7.f + cubeDisp)*0.0035, -0.1f, -7.f + cubeDisp));
    modelMat = T * R * S;
    modelMatInv = glm::transpose(glm::inverse(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
    glUniformMatrix4fv(glGetUniformLocation(gProgram[3], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
    glUniform1i(glGetUniformLocation(gProgram[3], "curr"), 2);
    
    if (collided != 2)
    	drawModel(3);
    
}





void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    
    // rendering
    
    renderSky();
    
    renderBunny();
    
    renderTiles();
    
    renderCheckpoints();
    
    // rendering over
    
    
    
    assert(glGetError() == GL_NO_ERROR);
	
	
	
	
	
	
	
	// ALY: rest of the checks and updates
	
	
	
	
	if (dead) { //if dead
		renderText("Score: " + to_string((int)(score / 2)), 0, 720, 0.75, glm::vec3(1, 0, 0));
	} // if dead over
	
	else { //if not dead (alive)
	if (rHeld) {
		score = 0;
		speed = 1;
		hop = 0;
		hoppingSpeed = 1;
		laying = 0;
		roll = 0;
		rollingSpeed = 1;
		fill(rolled.begin(), rolled.end(), 0);
		cubeDisp = 0;
		cubeSpeed = 1;
		currYellow = rand() % 3;
		mov = 0;
		movSpeed = 1;
		collided = -1;
		spin = 0;
		spinSpeed = 1;
	}
	

	if (aHeld && !dHeld) {
		if (mov > -1.9)
			mov -= movSpeed*0.03;
	}
	
	else if (dHeld && !aHeld) {
		if (mov < 1.9)
			mov += movSpeed*0.03;
	}
	
	
    renderText("Score: " + to_string((int)(score / 2)), 0, 720, 0.75, glm::vec3(1, 1, 0));
    
    
    //collision testing:
    if (cubeDisp > 6) {
    	if (mov < -1.6) { //colliding with left
    		if (currYellow == 0) { //colliding with yellow
    			if (collided == -1) {
					collided = 0;
					score += 2000;
					spin += 0.01;
    			}
    		}
    		else { //colliding with red
				dead = true;
				hoppingSpeed = 0;
				laying = -90;
				collided = 0;
    		}
    	}
    	
    	else if (mov > -0.55 && mov < 0.55) { //colliding with mid
    		if (currYellow == 1) { //colliding with yellow
    			if (collided == -1) {
					collided = 1;
					score += 2000;
					spin += 0.01;
    			}
    		}
    		else { //colliding with red
				dead = true;
				hoppingSpeed = 0;
				laying = -90;
				collided = 1;
    		}
    	}
    	
    	else if (mov > 1.6) { //colliding with right
    		if (currYellow == 2) { //colliding with yellow
    			if (collided == -1) {
					collided = 2;
					score += 2000;
					spin += 0.01;
    			}
    		}
    		else { //colliding with red
				dead = true;
				hoppingSpeed = 0;
				laying = -90;
				collided = 2;
    		}
    	}
    	
    }
    
    
    // values
	score += speed;
	speed += 0.0015;
	
	// bunny calculation
	hop += hoppingSpeed*0.04;
	
	if (hoppingSpeed > 0)
		hoppingSpeed += 0.0005;
		
	else hoppingSpeed -= 0.0005;
	
	if (hop > 0.5) {
		hop = 0.5;
		hoppingSpeed = -hoppingSpeed;
	}
	
	else if (hop < 0) {
		hop = 0;
		hoppingSpeed = -hoppingSpeed;
	}
	
	// floor calculation
	roll += rollingSpeed*0.1;
	rollingSpeed += 0.001;
	
	cubeDisp += 0.06*rollingSpeed;
	cubeSpeed += 0.001;
	
	if (cubeDisp > 7) {
		cubeDisp = -10;
		currYellow = rand() % 3;
		collided = -1;
	}
	
	movSpeed += 0.001;
	
	if (spin > 360) {
		spin = 0;
	}
	
	else if (spin > 0) {
		spin += spinSpeed*5;
	}
	
	spinSpeed+= 0.001;
		
	} //if alive over
	
	
	
	// ALY over
	
    assert(glGetError() == GL_NO_ERROR);

}













void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //ALY: setting inputs
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        rHeld = true;
        dead = false;
        laying = 0;
    }
    
    else if (key == GLFW_KEY_R && action == GLFW_RELEASE)
    {
        rHeld = false;
        dead = false;
    }
    
    else if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        aHeld = true;
    }
    
    else if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        aHeld = false;
    }
    
    else if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        dHeld = true;
    }
    
    else if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        dHeld = false;
    }
    
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

