#include "Shader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <stdexcept>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) : m_id(0) {
    const std::string vertexSource = loadShaderSource(vertexPath);
    const std::string fragmentSource = loadShaderSource(fragmentPath);
    create(vertexSource.c_str(), fragmentSource.c_str());
}

Shader::Shader(const char* vertexSource, const char* fragmentSource) : m_id(0) {
    create(vertexSource, fragmentSource);
}

Shader::~Shader() {
    if (m_id != 0) glDeleteProgram(m_id);
}

void Shader::use() {
    glUseProgram(m_id);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const {
    glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, glm::value_ptr(vec));
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

std::string Shader::loadShaderSource(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to open shader: " + path);
    std::stringstream source;
    source << file.rdbuf();
    return source.str();
}

void Shader::create(const char* vertexSource, const char* fragmentSource) {
    const auto compile = [](GLenum type, const char* source) {
        const GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[1024] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error(std::string("Shader compilation failed: ") + log);
        }
        return shader;
    };

    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource);
    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint success = GL_FALSE;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024] = {};
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        glDeleteProgram(m_id);
        m_id = 0;
        throw std::runtime_error(std::string("Shader linking failed: ") + log);
    }
}