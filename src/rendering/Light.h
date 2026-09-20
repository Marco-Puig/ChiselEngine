#pragma once
#include "scene/Node.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

class Light : public Node {
public:
    Light(const std::string& name, glm::vec3 color) : Node(name), m_color(color) {}
    virtual ~Light() = default;

    glm::vec3 getColor() const { return m_color; }
    void setColor(glm::vec3 color) { m_color = color; }

protected:
    glm::vec3 m_color;
};

class DirectionalLight : public Light {
public:
    DirectionalLight(const std::string& name, glm::vec3 direction)
        : Light(name, glm::vec3(1.0f)) {
        setDirection(direction);
    }

    glm::vec3 getDirection() const {
        return glm::normalize(glm::vec3(getWorldTransform() *
                                         glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
    }
    void setDirection(glm::vec3 direction) {
        if (glm::length(direction) <= 0.0001f)
            return;
        const glm::vec3 from(0.0f, 0.0f, -1.0f);
        const glm::vec3 to = glm::normalize(direction);
        const float cosine = glm::dot(from, to);
        if (cosine < -0.9999f) {
            setRotation(glm::angleAxis(glm::pi<float>(),
                                        glm::vec3(0.0f, 1.0f, 0.0f)));
            return;
        }
        const glm::vec3 axis = glm::cross(from, to);
        setRotation(glm::normalize(glm::quat(1.0f + cosine,
                                              axis.x, axis.y, axis.z)));
    }
    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = glm::max(0.0f, intensity); }
    float getExposure() const { return m_exposure; }
    void setExposure(float exposure) { m_exposure = exposure; }
    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = glm::max(0.1f, radius); }

private:
    float m_intensity = 1.0f;
    float m_exposure = 0.0f;
    float m_radius = 10.0f;
};

class PointLight : public Light {
public:
    PointLight(const std::string& name, glm::vec3 position)
        : Light(name, glm::vec3(1.0f)) {
        setPosition(position);
    }

    glm::vec3 getPosition() const {
        return glm::vec3(getWorldTransform()[3]);
    }
    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = glm::max(0.0f, intensity); }
    float getExposure() const { return m_exposure; }
    void setExposure(float exposure) { m_exposure = exposure; }
    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = glm::max(0.1f, radius); }

private:
    float m_intensity = 1.0f;
    float m_exposure = 0.0f;
    float m_radius = 10.0f;
};

