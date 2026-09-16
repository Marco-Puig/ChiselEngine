#pragma once
#include "Light.h"

class DirectionalLight : public Light {
public:
    DirectionalLight(const std::string& name, glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f)) 
        : Light(name), m_direction(direction) {}

    void setDirection(glm::vec3 dir) { m_direction = glm::normalize(dir); }
    glm::vec3 getDirection() const { return m_direction; }

    LightType getType() const override { return LightType::Directional; }

private:
    glm::vec3 m_direction;
};
