#pragma once

#include <windows.h>
#include <GLFW/glfw3.h>

class Window {
public:
    static Window& getInstance() {
        static Window instance;
        return instance;
    }

    void init(int width, int height);
    void shutdown();

    GLFWwindow* getHandle() const { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    HWND getHWND() const { return m_hWnd; }

private:
    Window() = default;
    GLFWwindow* m_window = nullptr;
    HWND m_hWnd = nullptr;
    int m_width = 1160;
    int m_height = 1100;
};
