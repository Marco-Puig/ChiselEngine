#include "Rendering/RenderSystem.h"
#include <iostream>
#include <vector>
#include <string>
#include <stb_image.h>

// Helper for shader compilation
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::cerr << "Shader compile error: " << log << std::endl;
    }
    return shader;
}

GLuint createProgram(const char* vs_src, const char* fs_src) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void RenderSystem::init() {
    // This would normally use a Shader class to load files
    // For now, mimicking the original app_init logic
    m_appShaderProgram = 0; // Should load from files
    m_appVAO = 0;
    m_appVBO = 0;
    m_appEBO = 0;
    
    // Skybox setup would go here, using m_skyboxVAO, etc.
}

GLuint RenderSystem::loadCubemap(std::vector<std::string> faces) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);
    int width, height, nrChannels;
    for (GLuint i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void RenderSystem::shutdown() {
    glDeleteProgram(m_appShaderProgram);
    glDeleteVertexArrays(1, &m_appVAO);
    glDeleteBuffers(1, &m_appVBO);
    glDeleteBuffers(1, &m_appEBO);
    glDeleteProgram(m_skyboxShaderProgram);
    glDeleteTextures(1, &m_cubemapTexture);
}
