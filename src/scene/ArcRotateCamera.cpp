#include "ArcRotateCamera.h"
#include "platform/Window.h"
#include "platform/SceneEditor.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

ArcRotateCamera::ArcRotateCamera(const std::string& name, Window* window)
    : Node(name), m_window(window) {}

void ArcRotateCamera::attach(Window& window) {
    m_window = &window;
    int width = 1;
    int height = 1;
    glfwGetFramebufferSize(window.getHandle(), &width, &height);
    setAspectRatio(static_cast<float>(width) / static_cast<float>(std::max(height, 1)));
    glfwGetCursorPos(window.getHandle(), &m_lastMouseX, &m_lastMouseY);
    m_mouseInitialized = true;
}

void ArcRotateCamera::setAspectRatio(float aspectRatio) {
    if (aspectRatio > 0.0f)
        m_aspectRatio = aspectRatio;
}

void ArcRotateCamera::setLimits(float minRadius, float maxRadius) {
    m_minRadius = std::max(0.01f, minRadius);
    m_maxRadius = std::max(m_minRadius, maxRadius);
    m_radius = std::clamp(m_radius, m_minRadius, m_maxRadius);
}

void ArcRotateCamera::addScroll(double yOffset) {
    m_pendingScroll += yOffset;
}

void ArcRotateCamera::update(float deltaTime) {
    (void)deltaTime;
    if (m_window == nullptr)
        return;

    GLFWwindow* handle = m_window->getHandle();
    const ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || io.WantCaptureKeyboard ||
        SceneEditor::isGizmoCapturingMouse())
        return;
    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(handle, &mouseX, &mouseY);
    if (!m_mouseInitialized) {
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        m_mouseInitialized = true;
    }

    const double deltaX = mouseX - m_lastMouseX;
    const double deltaY = mouseY - m_lastMouseY;
    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    const bool left = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool middle = glfwGetMouseButton(handle, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const bool shift = glfwGetKey(handle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                       glfwGetKey(handle, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    m_radius -= static_cast<float>(m_pendingScroll) * m_zoomSpeed;
    m_pendingScroll = 0.0;

    if (left && !shift && !middle) {
        m_alpha -= static_cast<float>(deltaX) * m_orbitSpeed;
        m_beta -= static_cast<float>(deltaY) * m_orbitSpeed;
    } else if (middle || (left && shift)) {
        const glm::vec3 forward = glm::normalize(m_target - getOrbitPosition());
        const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        const glm::vec3 up = glm::normalize(glm::cross(right, forward));
        m_target += right * static_cast<float>(-deltaX) * m_panSpeed;
        m_target += up * static_cast<float>(deltaY) * m_panSpeed;
    }

    m_beta = std::clamp(m_beta, 0.05f, glm::pi<float>() - 0.05f);
    m_radius = std::clamp(m_radius, m_minRadius, m_maxRadius);
}

glm::vec3 ArcRotateCamera::getOrbitPosition() const {
    const float sinBeta = std::sin(m_beta);
    return m_target + glm::vec3(
        m_radius * sinBeta * std::cos(m_alpha),
        m_radius * std::cos(m_beta),
        m_radius * sinBeta * std::sin(m_alpha));
}

glm::mat4 ArcRotateCamera::getViewMatrix() const {
    return glm::lookAt(getOrbitPosition(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 ArcRotateCamera::getProjectionMatrix() const {
    return glm::perspective(glm::radians(45.0f), m_aspectRatio, 0.1f, 100.0f);
}
