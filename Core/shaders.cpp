#include <shaders.h>

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

// Constructor that build the Shader Program from 2 different shaders
Shaders::Shaders(const char* vertexFile, const char* fragmentFile)
{
	// Read vertexFile and fragmentFile and store the strings
	std::string vertexCode = get_file_contents(vertexFile);
	std::string fragmentCode = get_file_contents(fragmentFile);

	// Convert the shader source strings into character arrays
	vertexShader = vertexCode.c_str();
	fragmentShader = fragmentCode.c_str();
}

// Load a vertex shader from a file
void Shaders::loadVertexShader(const char* vertexFile)
{
	// Read vertexFile and store the string
	std::string vertexCode = get_file_contents(vertexFile);
	// Convert the shader source string into a character array
	vertexShader = vertexCode.c_str();
}

// Load a fragment shader from a file
void Shaders::loadFragmentShader(const char* fragmentFile)
{
	// Read fragmentFile and store the string
	std::string fragmentCode = get_file_contents(fragmentFile);
	// Convert the shader source string into a character array
	fragmentShader = fragmentCode.c_str();
}

