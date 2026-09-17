#include "Engine.h"
#include "Rendering/RenderSystem.h"
#include <chrono>

void Engine::init() {
    m_window = std::make_unique<Window>(1280, 720, "ChiselEngine");
    RenderSystem::getInstance().init();
}

void Engine::run(IGame* game) {
    game->start();
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (!m_window->shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        m_window->pollEvents();
        game->update(dt);
        RenderSystem::getInstance().render(game->getSceneRoot());
        m_window->swapBuffers();
    }
}
