#include "Platform/Window.h"
#include "Platform/DevUI.h"
#include "Platform/PhysicsSystem.h"
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
    // If we can't init XR, we can still run in simulation mode
    if (!xr.init("Chisel Engine", 0x8C43)) {
        std::cout << "OpenXR failed to init, defaulting to simulation mode." << std::endl;
        xr.setSimulation(true);
    }

    // 4. Initialize Physics
    PhysicsSystem& physics = PhysicsSystem::getInstance();
    if (!physics.init()) {
        std::cerr << "Physics system failed to initialize" << std::endl;
    }

    // 5. Initialize DevUI
    devUI.init("Chisel Engine DevTools");
    devUI.addCheckbox("Simulated VR", false, [&](bool checked) {
        xr.setSimulation(checked);
        std::cout << "Simulation mode: " << (checked ? "ON" : "OFF") << std::endl;
    });

    // 5. Initialize Rendering

    RenderSystem& renderer = RenderSystem::getInstance();
    renderer.init();

    // 5. Start Game logic
    Game game;
    game.start();

    glEnable(GL_DEPTH_TEST);

    bool quit = false;
    while (!glfwWindowShouldClose(window.getHandle())) {
        devUI.update();
        glfwPollEvents();
        
        float deltaTime = 0.016f; // Simplified: should be calculated from glfwGetTime()
        physics.update(deltaTime);

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
    physics.shutdown();
    devUI.shutdown();
    window.shutdown();



    return 0;
}
