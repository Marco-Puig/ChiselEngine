// Tell OpenXR what platform code we'll be using
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define OPENGL_SWAPCHAIN_FORMAT 0x8C43 // GL_SRGB8_ALPHA8 for OpenXR/DX11 

#define WIN32_LEAN_AND_MEAN
#include <windows.h> // Windows functions such as Window creation

// OpenXR libs and includes
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

// OpenGL libs and includes
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// For texture loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> 

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/quaternion.hpp>

// Model loading
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <thread> // sleep_for
#include <vector> // dynamic array management for cube positions
#include <algorithm> // any_of

#include <tchar.h> // TCHAR type for parsing vertex and frag shader code

#include <cstdio> // parse - debug
#include <cmath> // sin, cos

#include <iostream> // std namespace
#include <sstream> // string conversions
#include <map> // key-value pairs
#include <string> // string manipulation

// Audio and sound lib from SFML Audio Module
#include <SFML/Audio.hpp>

using namespace std; // standard - std lib