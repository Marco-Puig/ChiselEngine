#include "Platform/Window.h"
#include <windows.h>
#include <iostream>

void Window::init(int width, int height) {
    m_width = width;
    m_height = height;

    if (!glfwInit()) {
        MessageBox(nullptr, _T("GLFW initialization failed\n"), _T("Error"), MB_OK);
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(m_width, m_height, "Chisel Engine - OpenGL 4.3", nullptr, nullptr);

    if (!m_window) {
        MessageBox(nullptr, _T("Window creation failed\n"), _T("Error"), MB_OK);
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(m_window);
    m_hWnd = (HWND)glfwGetWin32Window(m_window);
}

void Window::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}
