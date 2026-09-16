#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct app_transform_buffer_t {
    glm::mat4 world;
    glm::mat4 viewproj;
};

class RenderSystem {
public:
    static RenderSystem& getInstance() {
        static RenderSystem instance;
        return instance;
    }

    void init();
    void shutdown();
    GLuint loadCubemap(std::vector<std::string> faces);

    GLuint getShaderProgram() const { return m_appShaderProgram; }
    GLuint getVAO() const { return m_appVAO; }

private:
    RenderSystem() = default;

    GLuint m_appShaderProgram = 0;
    GLuint m_appUniformBuffer = 0;
    GLuint m_appVAO = 0;
    GLuint m_appUBO = 0;
    GLuint m_appVBO = 0;
    GLuint m_appEBO = 0;

    GLuint m_skyboxShaderProgram = 0;
    GLuint m_skyboxVAO = 0;
    GLuint m_skyboxVBO = 0;
    GLuint m_skyboxEBO = 0;
    GLuint m_cubemapTexture = 0;
};
