#include "Rendering/RenderSystem.h"
#include <glm/glm.hpp>

void RenderSystem::addLight(Light* light) {
    m_lights.push_back(light);
}

void RenderSystem::clearLights() {
    for (auto light : m_lights) {
        delete light;
    }
    m_lights.clear();
}

void RenderSystem::updateLightingUniforms(GLuint shaderProgram) {
    int lightCount = 0;
    for (auto light : m_lights) {
        if (light->getType() == LightType::Directional) {
            // This is a simplified implementation for a single directional light
            // In a full engine, you'd use a UBO or an array of lights
            std::string colorUniform = "uLightColor";
            std::string dirUniform = "uLightDir";
            
            glm::vec3 color = light->getColor() * light->getIntensity();
            
            // We cast to DirectionalLight to get the direction
            DirectionalLight* dirLight = static_cast<DirectionalLight*>(light);
            glm::vec3 dir = dirLight->getDirection();

            glUniform3fv(glGetUniformLocation(shaderProgram, colorUniform.c_str()), 1, &color[0]);
            glUniform3fv(glGetUniformLocation(shaderProgram, dirUniform.c_str()), 1, &dir[0]);
            
            lightCount++;
        }
    }
}
