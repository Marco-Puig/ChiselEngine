#pragma once
#include <string>
#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();
    
    GLFWwindow* getHandle() const { return m_window; }

private:
    GLFWwindow* m_window;
};
