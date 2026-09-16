#pragma once
#include <glm/glm.hpp>
#include <string>

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    Light(const std::string& name) : m_name(name) {}
    virtual ~Light() = default;

    void setColor(glm::vec3 color) { m_color = color; }
    glm::vec3 getColor() const { return m_color; }

    void setIntensity(float intensity) { m_intensity = intensity; }
    float getIntensity() const { return m_intensity; }

    const std::string& getName() const { return m_name; }
    virtual LightType getType() const = 0;

protected:
    std::string m_name;
    glm::vec3 m_color = glm::vec3(1.0f);
    float m_intensity = 1.0f;
};
