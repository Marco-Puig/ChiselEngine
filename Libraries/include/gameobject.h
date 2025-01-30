#pragma once

#include <engine.h> // Engine for OpenGL and OpenXR bindings and model loading

struct Transform {
	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;
};

struct Texture {
	GLuint id;
	std::string type;
	std::string path;
};

class Model {
public:
	GLuint vao;       // Vertex Array Object
	GLuint vbo;       // Vertex Buffer Object
	GLuint ebo;       // Element Buffer Object
	size_t indexCount; // Number of indices
	GLuint textureID; // Add a texture ID
	void loadModel(const std::string& objPath, const std::string& texturePath);
	void drawModel(const Transform modelTransform);
	void drawModel(); // Overloaded drawModel function
	void cleanupModel();
	GLuint loadTexture(const std::string& path);
};

Transform defaultTransform;
glm::mat4 transformToMat4(const Transform& transform);
Transform mat4ToTransform(const glm::mat4& mat);