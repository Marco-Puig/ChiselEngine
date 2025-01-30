#include <gameobject.h>

void Model::loadModel(const std::string& objPath, const std::string& texturePath = "") {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(objPath,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene || !scene->HasMeshes()) {
		//Error loading model => exit
		exit(EXIT_FAILURE);
	}

	aiMesh* mesh = scene->mMeshes[0];

	std::vector<float> vertices;
	std::vector<uint32_t> indices;

	// Extract vertex data: position (3), normal (3), tex coords (2)
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
		aiVector3D pos = mesh->mVertices[i];
		aiVector3D norm = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f, 0.0f, 0.0f);
		aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

		vertices.insert(vertices.end(), {
			pos.x, pos.y, pos.z,       // Position
			norm.x, norm.y, norm.z,    // Normal
			texCoord.x, texCoord.y     // Texture coordinates
			});
	}

	// Extract indices
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	// Load texture if provided
	GLuint textureID = !texturePath.empty() ? loadTexture(texturePath) : 0;

	// Generate VAO, VBO, and EBO
	GLuint vao, vbo, ebo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	// Bind VAO
	glBindVertexArray(vao);

	// Bind and fill VBO
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	// Bind and fill EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	// Configure vertex attributes
	glEnableVertexAttribArray(0); // Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1); // Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(2); // Texture coordinate attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	// Unbind VAO (optional, to avoid accidental changes)
	glBindVertexArray(0);

	// Store the model data in the Model object instance that called this function
	*this = { vao, vbo, ebo, indices.size(), textureID };
}


GLuint Model::loadTexture(const std::string& path) {
	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height, nrChannels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

	if (data) {
		GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	stbi_image_free(data);
	return textureID;
}

glm::mat4 transformToMat4(const Transform& transform) {
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), transform.position);
	glm::mat4 rotationMatrix = glm::mat4_cast(transform.rotation);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), transform.scale);

	return translationMatrix * rotationMatrix * scaleMatrix;
}

Transform mat4ToTransform(const glm::mat4& mat) {
	Transform transform;

	// Extract translation
	transform.position = glm::vec3(mat[3]);

	// Extract scale by calculating the length of the basis vectors
	transform.scale = glm::vec3(
		glm::length(glm::vec3(mat[0])),
		glm::length(glm::vec3(mat[1])),
		glm::length(glm::vec3(mat[2]))
	);

	// Remove scale from the matrix to extract the rotation
	glm::mat4 rotationMatrix = mat;
	rotationMatrix[0] /= transform.scale.x;
	rotationMatrix[1] /= transform.scale.y;
	rotationMatrix[2] /= transform.scale.z;

	transform.rotation = glm::quat_cast(rotationMatrix);

	return transform;
}

void Model::drawModel(const Transform modelTransform) {

	Model model = *this;
	glUseProgram(app_shader_program);

	// Convert Transform to a matrix
	glm::mat4 modelMatrix = transformToMat4(modelTransform);

	// Update transformation matrices
	app_transform_buffer_t modelTransformBuffer;
	modelTransformBuffer.viewproj = transform_buffer.viewproj;
	modelTransformBuffer.world = modelMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(app_transform_buffer_t), &modelTransformBuffer);

	// Bind the model's texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model.textureID);
	glUniform1i(glGetUniformLocation(app_shader_program, "texture_diffuse"), 0);

	// Draw the model
	glBindVertexArray(model.vao);
	glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}

void Model::drawModel() {

	const Transform modelTransform = defaultTransform;
	Model model = *this;

	glUseProgram(app_shader_program);

	// Convert Transform to a matrix
	glm::mat4 modelMatrix = transformToMat4(modelTransform);

	// Update transformation matrices
	app_transform_buffer_t modelTransformBuffer;
	modelTransformBuffer.viewproj = transform_buffer.viewproj;
	modelTransformBuffer.world = modelMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, app_uniform_buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(app_transform_buffer_t), &modelTransformBuffer);

	// Bind the model's texture
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, model.textureID);
	glUniform1i(glGetUniformLocation(app_shader_program, "texture_diffuse"), 0);

	// Draw the model
	glBindVertexArray(model.vao);
	glDrawElements(GL_TRIANGLES, model.indexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glUseProgram(0);
}


void Model::cleanupModel() {
	Model model = *this;
	glDeleteBuffers(1, &model.vbo);
	glDeleteBuffers(1, &model.ebo);
	glDeleteVertexArrays(1, &model.vao);
}