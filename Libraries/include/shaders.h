#pragma once

#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>

std::string get_file_contents(const char* filename);

class Shaders
{
public:
	const char* vertexShader;
	const char* fragmentShader;

	// Constructor that build the Shader Program from 2 different shaders
	Shaders(const char* vertexFile, const char* fragmentFile);
	void loadVertexShader(const char* vertexFile);
	void loadFragmentShader(const char* fragmentFile);
};