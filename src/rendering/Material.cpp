#pragma once
#include "Shader.h"
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>

// TODO: Create material loading support for API

class Material {
public:
    Material(std::shared_ptr<Shader> shader) : m_shader(shader) {}

    void setVec3(const std::string& name, const glm::vec3& value) { m_vec3Props[name] = value; }
    void setFloat(const std::string& name, float value) { m_floatProps[name] = value; }
    void setTexture(const std::string& name, GLuint textureID) { m_textures[name] = textureID; }

    void bind() {
        if (!m_shader) return;
        m_shader->use();
        int textureUnit = 0;
        for (const auto& [name, id] : m_textures) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, id);
            m_shader->setInt(name, textureUnit);
            textureUnit++;
        }
        for (const auto& [name, val] : m_vec3Props) m_shader->setVec3(name, val);
        for (const auto& [name, val] : m_floatProps) m_shader->setFloat(name, val);
    }

    std::shared_ptr<Shader> getShader() const { return m_shader; }

private:
    std::shared_ptr<Shader> m_shader;
    std::unordered_map<std::string, glm::vec3> m_vec3Props;
    std::unordered_map<std::string, float> m_floatProps;
    std::unordered_map<std::string, GLuint> m_textures;
};