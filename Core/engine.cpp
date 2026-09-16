#include "Platform/Window.h"
#include "XR/XRManager.h"
#include "Rendering/RenderSystem.h"
#include "Scene/Game.h"
#include <iostream>
#include <thread>

int main() {
    // 1. Initialize Window/Platform
    Window& window = Window::getInstance();
    window.init(1160, 1100);

    // 2. Load OpenGL Functions
    if (!gladLoadGL()) {
        std::cerr << "Failed to load OpenGL functions" << std::endl;
        return 1;
    }

    // 3. Initialize XR
    XRManager& xr = XRManager::getInstance();
    if (!xr.init("Chisel Engine", 0x8C43)) {
        std::cerr << "OpenXR initialization failed" << std::endl;
        return 1;
    }

    // 4. Initialize Rendering
    RenderSystem& renderer = RenderSystem::getInstance();
    renderer.init();

    // 5. Start Game logic
    Game game;
    game.start();

    glEnable(GL_DEPTH_TEST);

    bool quit = false;
    while (!glfwWindowShouldClose(window.getHandle())) {
        glfwPollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        xr.pollEvents(quit);
        if (quit) break;

        // XR Loop
        xr.pollActions();
        game.update();
        xr.renderFrame();

        glfwSwapBuffers(window.getHandle());
    }

    xr.shutdown();
    renderer.shutdown();
    window.shutdown();

    return 0;
}
