#include "Engine.h"
#include "rendering/RenderSystem.h"
#include "platform/SceneEditor.h"
#include "xr/XRManager.h"
#include "platform/PhysicsSystem.h"
#include "scene/ArcRotateCamera.h"
#include <glad/glad.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
#ifdef _WIN32
LONG WINAPI engineUnhandledException(EXCEPTION_POINTERS* info) noexcept {
    const DWORD code = info != nullptr && info->ExceptionRecord != nullptr
        ? info->ExceptionRecord->ExceptionCode : 0;
    char message[128] = {};
    std::snprintf(message, sizeof(message),
                  "[Engine] unhandled SEH exception: 0x%08lX\n",
                  static_cast<unsigned long>(code));
    std::cerr << message;
    std::cerr.flush();
    OutputDebugStringA(message);
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

[[noreturn]] void engineTerminate() noexcept {
    std::cerr << "[Engine] std::terminate called\n";
    std::cerr.flush();
#ifdef _WIN32
    OutputDebugStringA("[Engine] std::terminate called\n");
#endif
    std::abort();
}
}

void Engine::init() {
    std::set_terminate(engineTerminate);
#ifdef _WIN32
    SetUnhandledExceptionFilter(engineUnhandledException);
#endif
    m_window = std::make_unique<Window>(1600, 900, "ChiselEngine");
    RenderSystem::getInstance().init();
    XRManager::getInstance().init(*m_window);
    PhysicsSystem::getInstance().init();
    SceneEditor::getInstance().init(*m_window);
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
        SceneEditor::getInstance().beginFrame();
        SceneEditor::getInstance().updateGizmo(game->getSceneRoot(), game->getCamera());
        PhysicsSystem::getInstance().update(dt);
        game->update(dt);
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
        SceneEditor::getInstance().render(*m_window, xr, game->getSceneRoot(),
                                    game->getCamera(), game->getAnimator(), dt);
        m_window->swapBuffers();
    }

    SceneEditor::getInstance().shutdown();
    PhysicsSystem::getInstance().shutdown();
    xr.shutdown();
}
