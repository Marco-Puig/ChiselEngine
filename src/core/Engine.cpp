#include "Engine.h"
#include "rendering/RenderSystem.h"
#include "platform/DevUI.h"
#include "xr/XRManager.h"
#include "platform/PhysicsSystem.h"
#include "scene/ArcRotateCamera.h"
#include <glad/glad.h>
#include <chrono>

void Engine::init() {
    m_window = std::make_unique<Window>(1280, 720, "ChiselEngine");
    RenderSystem::getInstance().init();
    XRManager::getInstance().init(*m_window);
    PhysicsSystem::getInstance().init();
    DevUI::getInstance().init(*m_window);
}

void Engine::run(IGame* game) {
    game->start();
    XRManager& xr = XRManager::getInstance();
    if (game->getCamera() != nullptr) {
        game->getCamera()->attach(*m_window);
        m_window->setResizeTarget(game->getCamera());
        RenderSystem::getInstance().setDesktopCamera(game->getCamera());
    }
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (!m_window->shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        m_window->pollEvents();
        DevUI::getInstance().beginFrame();
        DevUI::getInstance().updateGizmo(game->getSceneRoot(), game->getCamera());
        game->update(dt);
        PhysicsSystem::getInstance().update(dt);
        xr.syncActions();
        if (xr.beginFrame()) {
            for (uint32_t eye = 0; eye < 2; ++eye) {
                glm::mat4 view;
                glm::mat4 projection;
                if (!xr.acquireView(eye, view, projection))
                    continue;
                GLuint framebuffer = 0;
                GLuint depthBuffer = 0;
                glGenFramebuffers(1, &framebuffer);
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                       GL_TEXTURE_2D, xr.getViewTexture(eye), 0);
                glGenRenderbuffers(1, &depthBuffer);
                glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                                      static_cast<GLsizei>(xr.getViewWidth(eye)),
                                      static_cast<GLsizei>(xr.getViewHeight(eye)));
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_RENDERBUFFER, depthBuffer);
                RenderSystem::getInstance().renderView(
                    game->getSceneRoot(), view, projection, framebuffer,
                    static_cast<int>(xr.getViewWidth(eye)),
                    static_cast<int>(xr.getViewHeight(eye)));
                glDeleteFramebuffers(1, &framebuffer);
                glDeleteRenderbuffers(1, &depthBuffer);
                xr.releaseView(eye);
            }
            xr.endFrame();
        } else {
            RenderSystem::getInstance().render(game->getSceneRoot());
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DevUI::getInstance().render(*m_window, xr, game->getSceneRoot(),
                                    game->getCamera(), dt);
        m_window->swapBuffers();
    }

    DevUI::getInstance().shutdown();
    PhysicsSystem::getInstance().shutdown();
    xr.shutdown();
}
