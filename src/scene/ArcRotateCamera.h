#pragma once
#include "Node.h"
#include <glm/glm.hpp>

class Window;

class ArcRotateCamera : public Node {
public:
    ArcRotateCamera(const std::string& name, Window* window = nullptr);

    void attach(Window& window);
    void update(float deltaTime);
    void addScroll(double yOffset);
    void setAspectRatio(float aspectRatio);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    float getAlpha() const { return m_alpha; }
    float getBeta() const { return m_beta; }
    float getRadius() const { return m_radius; }
    glm::vec3 getTarget() const { return m_target; }
    glm::vec3 getOrbitPosition() const;

    void setTarget(const glm::vec3& target) { m_target = target; }
    void setLimits(float minRadius, float maxRadius);

private:
    Window* m_window = nullptr;
    glm::vec3 m_target{0.0f};
    float m_alpha = glm::radians(45.0f);
    float m_beta = glm::radians(60.0f);
    float m_radius = 6.0f;
    float m_minRadius = 1.0f;
    float m_maxRadius = 100.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_orbitSpeed = 0.005f;
    float m_zoomSpeed = 0.5f;
    float m_panSpeed = 0.01f;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_mouseInitialized = false;
    double m_pendingScroll = 0.0;
};
