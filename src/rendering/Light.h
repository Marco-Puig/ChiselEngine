#pragma once
#include <glm/glm.hpp>
#include <string>

class Light {
public:
    Light(const std::string& name, glm::vec3 color) : m_name(name), m_color(color) {}
    virtual ~Light() = default;

    const std::string& getName() const { return m_name; }
    glm::vec3 getColor() const { return m_color; }
    void setColor(glm::vec3 color) { m_color = color; }

protected:
    std::string m_name;
    glm::vec3 m_color;
};

class DirectionalLight : public Light {
public:
    DirectionalLight(const std::string& name, glm::vec3 direction) 
        : Light(name, glm::vec3(1.0f)), m_direction(direction) {}

    glm::vec3 getDirection() const { return m_direction; }
    void setDirection(glm::vec3 dir) { m_direction = dir; }

private:
    glm::vec3 m_direction;
};

class PointLight : public Light {
public:
    PointLight(const std::string& name, glm::vec3 position) 
        : Light(name, glm::vec3(1.0f)), m_position(position) {}

    glm::vec3 getPosition() const { return m_position; }
    void setPosition(glm::vec3 pos) { m_position = pos; }

private:
    glm::vec3 m_position;
};
