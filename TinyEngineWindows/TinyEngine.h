#pragma once
#include <functional>
#include <deque>
using Handle = std::function<void()>;
#include <initializer_list>
using slist = std::initializer_list<std::string>;

//Interface Dependencies (DearImGUI)
#include "include/imgui/imgui.h"
#include "include/imgui/imgui_impl_sdl.h"
#include "include/imgui/imgui_impl_opengl3.h"
//#define IMGUI_IMPL_OPENGL_LOADER_GLEW
//#define GLEW_STATIC

//Drawing Dependencies
#include <GL/glew.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"

//File IO
#include <sstream>
#include <iostream>
#include <fstream>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

//Helpers
#include "include/helpers/helper.h"
#include "include/helpers/ease.h"
#include "include/helpers/color.h"
#include "include/helpers/draw.h"
#include "include/helpers/image.h"
#include "include/helpers/timer.h"

//Utility Classes for the Engine
//#include "include/utility/texture.cpp"
//#include "include/utility/shader.cpp"
//#include "include/utility/sprite.cpp"
//#include "include/utility/particle.cpp"
//#include "include/utility/billboard.cpp"
//#include "include/utility/model.cpp"

//#include "include/view.cpp"
//#include "include/event.cpp"
//#include "include/audio.cpp"

/* TINY ENGINE */

class Billboard {
public:
	Billboard(int width, int height, bool depthOnly) {
		setup();
		drawable(width, height, depthOnly);
	};

	~Billboard() {
		cleanup();
	}

	//Rendering Data
	unsigned int WIDTH, HEIGHT;
	GLuint vao, vbo[2];
	void setup();
	void cleanup();
	bool depth;

	//Vertex and Texture Positions
	const GLfloat vert[8] = { -1.0, -1.0, -1.0,  1.0,  1.0, -1.0,  1.0,  1.0 };
	const GLfloat tex[8] = { 0.0,  0.0,  0.0,  1.0,  1.0,  0.0,  1.0,  1.0 };

	//Rendering Position
	glm::mat4 model = glm::mat4(1.0f);                  //Model Matrix
	void move(glm::vec2 pos, glm::vec2 scale);

	//Textures and FBO
	GLuint fbo;       //We need an FBO to render scene to screen
	GLuint texture;
	GLuint depthTexture;

	void raw(SDL_Surface* s);
	bool drawable(int width, int height, bool depthOnly);

	void target();                //Billboard as render target
	void target(glm::vec3 clear); //Billboard as render target with clear color
	void render();                //Render this billboard
};

void Billboard::setup() {
	//Setup the VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Setup the VBOs
	glGenBuffers(2, &vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), &vert[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), &tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//Generate Textures
	glGenTextures(1, &texture);
	glGenTextures(1, &depthTexture);
}

void Billboard::cleanup() {
	glDeleteTextures(1, &texture);
	glDeleteTextures(1, &depthTexture);
	glDeleteFramebuffers(1, &fbo);
	glDisableVertexAttribArray(vao);
	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
}

void Billboard::raw(SDL_Surface* s) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Billboard::move(glm::vec2 pos, glm::vec2 scale) {
	model = glm::translate(glm::mat4(1.0), glm::vec3(2.0*pos.x - 1.0 + scale.x, 2.0*pos.y - 1.0 + scale.y, 0.0));
	model = glm::scale(model, glm::vec3(scale.x, scale.y, 1.0));
}

bool Billboard::drawable(int width, int height, bool depthOnly) {
	glGenFramebuffers(1, &fbo); //Frame Buffer Object for drawing
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	depth = depthOnly;
	WIDTH = width;
	HEIGHT = height;

	if (!depthOnly) {
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	}

	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	if (depth) glDrawBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Failed to construct framebuffer object." << std::endl;
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void Billboard::target() {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, WIDTH, HEIGHT);
	if (depth) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Billboard::target(glm::vec3 clear) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(clear.x, clear.y, clear.z, 1.0f); //Blue
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Billboard::render() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
class Model {
public:
	Model() { setup(); };

	Model(std::function<void(Model* m)> c) {
		setup();
		construct(c);
	};

	~Model() {
		glDisableVertexAttribArray(vao);
		glDeleteBuffers(3, vbo);
		glDeleteBuffers(1, &ibo);
		glDeleteVertexArrays(1, &vao);
	}

	std::vector<GLfloat>  positions;
	std::vector<GLfloat>  normals;
	std::vector<GLfloat>  colors;
	std::vector<GLuint>   indices;

	GLuint vbo[3], vao, ibo;

	glm::mat4 model = glm::mat4(1.0f);  //Model Matrix
	glm::vec3 pos = glm::vec3(0.0f);    //Model Position

	void setup();
	void update();
	void construct(std::function<void(Model* m)> constructor) {
		positions.clear();  //Clear all Data
		normals.clear();
		colors.clear();
		indices.clear();

		(constructor)(this);  //Call user-defined constructor
		update();             //Update VAO / VBO / IBO
	};

	void translate(const glm::vec3 &axis);
	void rotate(const glm::vec3 &axis, float angle);

	void render(GLenum T);
};

void Model::setup() {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(3, vbo);
	glGenBuffers(1, &ibo);
}

void Model::update() {
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);      //Positions
	glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), &positions[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);      //Normals
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat), &normals[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);      //Colors
	glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), &colors[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); //Indices
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);
}

void Model::translate(const glm::vec3 &axis) {
	model = glm::translate(model, axis);
	pos += axis;
}

void Model::rotate(const glm::vec3 &axis, float angle) {
	model = glm::translate(glm::rotate(glm::translate(model, -pos), angle, axis), pos);
}

void Model::render(GLenum T) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glDrawElements(T, indices.size(), GL_UNSIGNED_INT, 0);
}
class Particle {
public:
	Particle() {       //Construct from an SDL Surface
		setup();
	};

	~Particle() {
		cleanup();
	}

	//Rendering Data
	GLuint vao, vbo[2], instance;

	void setup();
	void update();
	void cleanup();


	//Vertex and Texture Positions
	const GLfloat vert[12] = { -1.0, -1.0,  0.0,
							 -1.0,  1.0,  0.0,
							  1.0, -1.0,  0.0,
							  1.0,  1.0,  0.0 };

	const GLfloat tex[8] = { 0.0,  1.0,
							  0.0,  0.0,
							  1.0,  1.0,
							  1.0,  0.0 };

	//Rendering Position
	std::vector<glm::mat4> models;
	void render();                //Render this billboard
};

void Particle::setup() {
	//Setup the VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Setup the VBOs
	glGenBuffers(2, &vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), &vert[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), &tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//Setup Instance VBO
	glGenBuffers(1, &instance);
}

void Particle::update() {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, instance);
	glBufferData(GL_ARRAY_BUFFER, models.size() * sizeof(glm::mat4), &models[0], GL_STATIC_DRAW);

	std::size_t vec4Size = sizeof(glm::vec4);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
}

void Particle::cleanup() {
	//Delete Textures and VAO
	glDisableVertexAttribArray(vao);
	glDeleteBuffers(2, vbo);
	glDeleteBuffers(1, &instance);
	glDeleteVertexArrays(1, &vao);
}

void Particle::render() {
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, models.size());
}
class Shader {
public:
	Shader(std::string vs, std::string fs, slist _list) {
		setup(vs, fs);        //Setup the Shader
		for (auto &n : _list)  //Add all Attributes of Shader
			addInput(&n - _list.begin(), n);
		link();
	};

	~Shader() {
		glDeleteProgram(shaderProgram);
		glDeleteShader(fragmentShader);
		glDeleteShader(vertexShader);
	}

	GLuint shaderProgram;   //Shader Program ID
	GLuint vertexShader, fragmentShader;

	void setup(std::string vs, std::string fs);
	void addInput(int pos, std::string attrName);
	void cleanup();
	int  addProgram(std::string fileName, GLenum shaderType);  //General Shader Addition
	std::string readGLSLFile(std::string fileName, int32_t &size); //Read File
	void compile(GLuint shader);  //Compile and Add File
	void link();                  //Link the entire program
	void use();                   //Use the program

	// Uniform Setters
	void setBool(std::string name, bool value);
	void setInt(std::string name, int value);
	void setFloat(std::string name, float value);
	void setVec2(std::string name, const glm::vec2 vec);
	void setVec3(std::string name, const glm::vec3 vec);
	void setVec4(std::string name, const glm::vec4 vec);
	void setMat3(std::string name, const glm::mat3 mat);
	void setMat4(std::string name, const glm::mat4 mat);
};

void Shader::setup(std::string vs, std::string fs) {
	shaderProgram = glCreateProgram();  //Generate Shader

	boost::filesystem::path data_dir(boost::filesystem::current_path());
	vertexShader = addProgram((data_dir / vs).string(), GL_VERTEX_SHADER);
	fragmentShader = addProgram((data_dir / fs).string(), GL_FRAGMENT_SHADER);
}

int Shader::addProgram(std::string fileName, GLenum shaderType) {
	//Read Shader Program from Source
	char* src; int32_t size;
	std::string result = readGLSLFile(fileName, size);
	src = const_cast<char*>(result.c_str());

	//Create and Compile Shader
	int shaderID = glCreateShader(shaderType);
	glShaderSource(shaderID, 1, &src, &size);
	compile(shaderID);

	return shaderID;
}

void Shader::addInput(int pos, std::string attrName) {
	glBindAttribLocation(shaderProgram, pos, attrName.c_str());
}

void Shader::compile(GLuint shader) {
	glCompileShader(shader);

	int success, maxLength;  ///Error Handling
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success) {
		glAttachShader(shaderProgram, shader);
		return;
	}

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
	char* shaderInfoLog = new char[maxLength];
	glGetShaderInfoLog(shader, maxLength, &maxLength, shaderInfoLog);
	std::cout << "Linker error message : " << shaderInfoLog << std::endl;
	delete shaderInfoLog;
}

void Shader::link() {
	glLinkProgram(shaderProgram);

	int success, maxLength;  //Error Handling
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, (int *)&success);
	if (success) return; //Yay

	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);
	char* shaderProgramInfoLog = new char[maxLength];
	glGetProgramInfoLog(shaderProgram, maxLength, &maxLength, shaderProgramInfoLog);
	std::cout << "Linker error message: " << shaderProgramInfoLog << std::endl;
	delete shaderProgramInfoLog;
}

void Shader::use() {
	glUseProgram(shaderProgram);
}

std::string Shader::readGLSLFile(std::string file, int32_t &size) {
	std::ifstream t;
	std::string fileContent;

	t.open(file);     //Read GLSL File Contents
	if (t.is_open()) {
		std::stringstream buffer;
		buffer << t.rdbuf();
		fileContent = buffer.str();
	}
	else std::cout << "File opening failed" << std::endl;
	t.close();

	size = fileContent.length();  //Set the Size
	return fileContent;
}

/* Uniform Setters */

void Shader::setBool(std::string name, bool value) {
	glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

void Shader::setInt(std::string name, int value) {
	glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

void Shader::setFloat(std::string name, float value) {
	glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

void Shader::setVec2(std::string name, const glm::vec2 vec) {
	glUniform2fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &vec[0]);
}

void Shader::setVec3(std::string name, const glm::vec3 vec) {
	glUniform3fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &vec[0]);
}

void Shader::setVec4(std::string name, const glm::vec4 vec) {
	glUniform4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, &vec[0]);
}

void Shader::setMat3(std::string name, const glm::mat3 mat) {
	glUniformMatrix3fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(std::string name, const glm::mat4 mat) {
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
class Sprite {
public:
	Sprite(SDL_Surface* s) {       //Construct from an SDL Surface
		setup();
		update(s);
	};

	Sprite() {       //Only construct (empty texture)
		setup();
	};

	~Sprite() {
		cleanup();
	}

	//Rendering Data
	GLuint vao, vbo[2];
	void setup();
	void cleanup();

	//Vertex and Texture Positions
	const GLfloat vert[12] = { -1.0, -1.0,  0.0,
							 -1.0,  1.0,  0.0,
							  1.0, -1.0,  0.0,
							  1.0,  1.0,  0.0 };

	const GLfloat tex[8] = { 0.0,  1.0,
							  0.0,  0.0,
							  1.0,  1.0,
							  1.0,  0.0 };

	//Rendering Position
	glm::vec3 pos = glm::vec3(0.0);
	glm::mat4 model = glm::mat4(1.0f);                  //Model Matrix
	void move(glm::vec2 pos, glm::vec2 scale);

	void update(SDL_Surface* TextureImage);
	void render();                //Render this billboard
};

void Sprite::setup() {
	//Setup the VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Setup the VBOs
	glGenBuffers(2, &vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), &vert[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), &tex[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void Sprite::cleanup() {
	//Delete Textures and VAO
	glDisableVertexAttribArray(vao);
	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
}

void Sprite::render() {
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
class Texture {
public:
	Texture(SDL_Surface* s) {       //Construct from an SDL Surface
		setup();
		raw(s);
	};

	Texture() {       //Only construct (empty texture)
		setup();
	};

	~Texture() {
		cleanup();
	}

	//Rendering Data
	GLuint texture;
	void setup();
	void cleanup();
	void raw(SDL_Surface* TextureImage);
};

void Texture::setup() {
	glGenTextures(1, &texture);
}

void Texture::cleanup() {
	glDeleteTextures(1, &texture);
}

void Texture::raw(SDL_Surface* s) {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, s->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

class View {
public:
	bool init(std::string windowName, int width, int height);
	void cleanup();

	unsigned int WIDTH, HEIGHT;

	SDL_Window* gWindow;        //Window Pointer
	SDL_GLContext gContext;     //Render Context

	ImGuiIO io;
	Handle interface;           //User defined Interface
	bool showInterface = false;
	void drawInterface();

	Handle pipeline;            //User defined Pipeline
	void render();
	void target(glm::vec3 clearcolor);  //Target main window for drawing

	//Flags
	bool fullscreen = false;
	bool vsync = true;
};

bool View::init(std::string _name, int _width, int _height) {
	WIDTH = _width;
	HEIGHT = _height;

	//Initialize the Window and Context
	gWindow = SDL_CreateWindow(_name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	if (gWindow == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return false;
	}
	SDL_SetWindowResizable(gWindow, SDL_TRUE);
	gContext = SDL_GL_CreateContext(gWindow);

	//Initialize OPENGL Stuff
	SDL_GL_SetSwapInterval(vsync);
	glewExperimental = GL_TRUE;
	glewInit();

	//Setup the Guy
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	io = ImGui::GetIO(); (void)io;

	ImGui_ImplSDL2_InitForOpenGL(gWindow, gContext);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGui::StyleColorsCustom();

	//Configure Global OpenGL State
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glLineWidth(1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void View::cleanup() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(gWindow);
}

void View::render() {

	//User-defined rendering pipeline
	(pipeline)();

	if (showInterface)
		drawInterface();

	SDL_GL_SwapWindow(gWindow); //Update Window
}

void View::drawInterface() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(gWindow);
	ImGui::NewFrame();

	(interface)();  //Draw user-defined interface
	//ImGui::ShowDemoWindow();

	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void View::target(glm::vec3 clearcolor) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(clearcolor.x, clearcolor.y, clearcolor.z, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

class Event {
private:
	SDL_Event in;

public:
	bool quit = false;
	void input();                   //Take inputs and add them to stack

	void handle(View &view);        //General Event Handler
	Handle handler;                 //User defined event Handler

	bool fullscreenToggle = false;

	std::deque<SDL_Event> keys;
	bool keyEventTrigger = false;
	std::deque<SDL_Event> scroll;  //Scrolling Motion Inputs
	SDL_Event windowEvent;         //Window Resizing Event
	bool windowEventTrigger = false;
	SDL_Event mouseEvent;          //Mouse Click Event
	bool mouseEventTrigger = false;
	SDL_Event moveEvent;           //Mouse Movement Events
	bool moveEventTrigger = false;
};

void Event::input() {
	if (SDL_PollEvent(&in) == 0) return;

	if (in.type == SDL_QUIT) quit = true;
	ImGui_ImplSDL2_ProcessEvent(&in);

	if (in.type == SDL_KEYUP) {
		if (in.key.keysym.sym == SDLK_F11) fullscreenToggle = true;
		else keys.push_front(in);
		return;
	}

	if (in.type == SDL_MOUSEWHEEL) {
		scroll.push_front(in);
		return;
	}

	if (in.type == SDL_MOUSEBUTTONDOWN ||
		in.type == SDL_MOUSEBUTTONUP) {
		mouseEvent = in;
		mouseEventTrigger = true;
		return;
	}

	if (in.type == SDL_WINDOWEVENT) {
		windowEvent = in;
		windowEventTrigger = true;
		return;
	}
}

void Event::handle(View &view)
{
	(handler)();  //Call user-defined handler first

	if (fullscreenToggle) {
		view.fullscreen = !view.fullscreen; //Toggle Fullscreen!
		if (!view.fullscreen) SDL_SetWindowFullscreen(view.gWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		else SDL_SetWindowFullscreen(view.gWindow, 0);
		fullscreenToggle = false;
	}

	if (windowEventTrigger && windowEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
		view.WIDTH = windowEvent.window.data1;
		view.HEIGHT = windowEvent.window.data2;
		windowEventTrigger = false;
	}

	if (!keys.empty() && keys.back().key.keysym.sym == SDLK_ESCAPE) {
		view.showInterface = !view.showInterface;
	}

	if (!scroll.empty()) scroll.pop_back();
	if (!keys.empty()) keys.pop_back();
}

/*
	WORK IN PROGRESS
*/

class Audio {
public:
	//Storage for unprocessed soundbytes
  //  std::vector<SoundByte> unprocessed;

	//Vector containing the guy
	Mix_Chunk *med = NULL;
	Mix_Chunk *hit = NULL;

	bool init();
	bool cleanup();
	void process();
};

bool Audio::init() {
	//Intialize Interface
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1) return false;

	/*
	  //Load the sound effects
	  med = Mix_LoadWAV( "resource/audio/acoustic.wav" );
	  hit = Mix_LoadWAV( "resource/audio/medium.wav" );

	  //If there was a problem loading the sound effects
	  if( med == NULL ) return false;
	  if( hit == NULL ) return false;
	*/

	//If everything loaded fine
	return true;
}

bool Audio::cleanup() {
	//Free the Chunks
	Mix_FreeChunk(med);
	Mix_FreeChunk(hit);

	//Close the Audio Interface
	Mix_CloseAudio();

	return true;
}

void Audio::process() {

	/*
	In the future, we can also save the position where the sound-effect was emitted
	-> to make sure that volume effects play a role.


	I need to be able to play sounds based on queues
	with a specific volume
	and maybe a bunch of other possible effects...
	*/
	/*

	while(!unprocessed.empty()){
	  if(unprocessed.back() != SOUND_NONE) Mix_PlayChannel( -1, hit, 0 );
	  //Play the corresponding sound-effect and remove it from the unprocessed list.
	  unprocessed.pop_back();
	}
	*/
}

namespace Tiny {

	//Main Engine Elements
	static View view;       //Window and Interface
	static Event event;     //Event Handler
	static Audio audio;     //Audio Processor

	bool init(std::string windowName, int width, int height) {
		//Initialize SDL Core
		if (SDL_Init(SDL_INIT_VIDEO) < 0) {
			printf("SDL could not initialize! Error: %s\n", SDL_GetError());
			return false;
		}

		//Initialize SDL_Image
		if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
			printf("SDL_Image could not initialize! Error: %s\n", IMG_GetError());
			return false;
		}

		//Initialize SDL_TTF
		TTF_Init();

		//Initialize the View
		if (!view.init(windowName, width, height)) {
			std::cout << "Failed to launch visual interface." << std::endl;
			return false;
		}

		if (!audio.init()) {
			std::cout << "Failed to launch audio interface." << std::endl;
			return false;
		}
	}

	void quit() {
		view.cleanup();
		audio.cleanup();
		TTF_Quit();
		SDL_Quit();
	};

	template<typename F, typename... Args>
	void loop(F function, Args&&... args) {
		while (!event.quit) {
			event.input();        //Handle Input
			event.handle(view);

			audio.process();      //Audio Processor

			function(args...);    //User-defined Game Loop

			view.render();        //Render View
		}
	};

	//End of Namespace
};