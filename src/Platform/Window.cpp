#include "Window.h"
#include <glad/glad.h>
#include <stdexcept>
#include <algorithm>
#include "scene/ArcRotateCamera.h"

namespace {
void framebufferSizeCallback(GLFWwindow* handle, int width, int height) {
    glViewport(0, 0, width, height);
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->getResizeCamera() != nullptr)
        window->getResizeCamera()->setAspectRatio(
            static_cast<float>(width) / static_cast<float>(std::max(height, 1)));
}

void scrollCallback(GLFWwindow* handle, double, double yOffset) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->getResizeCamera() != nullptr)
        window->getResizeCamera()->addScroll(yOffset);
}
}

Window::Window(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSwapInterval(1);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("Failed to initialize OpenGL loader");
    }
}

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::setVSync(bool enabled) {
    if (m_window == nullptr)
        return;
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setResizeTarget(ArcRotateCamera* camera) {
    m_resizeCamera = camera;
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    int width = 1;
    int height = 1;
    glfwGetFramebufferSize(m_window, &width, &height);
    if (camera != nullptr)
        camera->setAspectRatio(static_cast<float>(width) / static_cast<float>(std::max(height, 1)));
}
