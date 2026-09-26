// android_main.cpp
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>

#include <EGL/egl.h>
#include <GLES3/gl3.h>

// Engine headers
#include "Game.h"
#include "platform/FileLoader.h"
#include "platform/PhysicsSystem.h"
#include "xr/XRManager.h"
#include "rendering/RenderSystem.h"

// Note: Your CMake glad configuration must target OpenGL ES 3.x for Android.
// If glad is strictly configured for desktop GL, you will need to regenerate 
// it for GLES3 or rely directly on the <GLES3/gl3.h> NDK headers.
#include <glad/glad.h>

#include <memory>
#include <string>
#include <chrono>
#include <exception>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "ChiselEngine", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ChiselEngine", __VA_ARGS__)

struct AndroidState {
    android_app* app = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config = nullptr;
    bool initialized = false;
    bool running = false;
};

static AndroidState g_state;

bool initEGL(android_app* app) {
    g_state.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_state.display == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    if (!eglInitialize(g_state.display, nullptr, nullptr)) {
        LOGE("Failed to initialize EGL");
        return false;
    }

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_BLUE_SIZE,       8,
        EGL_GREEN_SIZE,      8,
        EGL_RED_SIZE,        8,
        EGL_ALPHA_SIZE,      8,
        EGL_DEPTH_SIZE,      24,
        EGL_STENCIL_SIZE,    8,
        EGL_NONE
    };

    EGLint numConfigs;
    if (!eglChooseConfig(g_state.display, configAttribs, &g_state.config, 1, &numConfigs) || numConfigs == 0) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    // Set native window pixel format to match EGL config
    EGLint format;
    eglGetConfigAttrib(g_state.display, g_state.config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(app->window, 0, 0, format);

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    g_state.context = eglCreateContext(g_state.display, g_state.config, EGL_NO_CONTEXT, contextAttribs);
    if (g_state.context == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    g_state.surface = eglCreateWindowSurface(g_state.display, g_state.config, app->window, nullptr);
    if (g_state.surface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }

    if (!eglMakeCurrent(g_state.display, g_state.surface, g_state.surface, g_state.context)) {
        LOGE("Failed to make EGL context current");
        return false;
    }

    LOGI("EGL initialized successfully");
    return true;
}

void shutdownEGL() {
    if (g_state.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_state.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_state.context != EGL_NO_CONTEXT) eglDestroyContext(g_state.display, g_state.context);
        if (g_state.surface != EGL_NO_SURFACE) eglDestroySurface(g_state.display, g_state.surface);
        eglTerminate(g_state.display);
    }
    g_state.display = EGL_NO_DISPLAY;
    g_state.surface = EGL_NO_SURFACE;
    g_state.context = EGL_NO_CONTEXT;
    g_state.config = nullptr;
}

void handleCommand(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (app->window != nullptr) {
                if (initEGL(app)) {
                    g_state.initialized = true;
                    g_state.running = true;
                    LOGI("Window initialized");
                }
            }
            break;

        case APP_CMD_TERM_WINDOW:
            g_state.running = false;
            shutdownEGL();
            g_state.initialized = false;
            LOGI("Window terminated");
            break;

        case APP_CMD_GAINED_FOCUS:
            g_state.running = true;
            break;

        case APP_CMD_LOST_FOCUS:
            g_state.running = false;
            break;

        case APP_CMD_PAUSE:
            g_state.running = false;
            break;

        case APP_CMD_RESUME:
            g_state.running = true;
            break;

        case APP_CMD_DESTROY:
            g_state.running = false;
            break;
    }
}

int32_t handleInput(android_app* app, AInputEvent* event) {
    // Input is handled via OpenXR on Quest, not Android input.
    return 0;
}

void android_main(android_app* app) {
    g_state.app = app;
    app->onAppCmd = handleCommand;
    app->onInputEvent = handleInput;

    LOGI("ChiselEngine starting on Android/Quest");

    // 1. Wire up the Asset Manager immediately so FileLoader can read from the APK
    FileLoader::setAssetManager(app->activity->assetManager);

    std::unique_ptr<Game> game;
    auto lastTime = std::chrono::high_resolution_clock::now();

    // Main loop
    while (true) {
        int events;
        android_poll_source* source;

        // Poll Android events
        while (ALooper_pollAll(g_state.running ? 0 : -1, nullptr, &events,
                               reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                LOGI("Engine shutting down");
                goto cleanup;
            }
        }

        if (!g_state.initialized || !g_state.running) {
            continue;
        }

        // Initialize GLAD and Game exactly once when EGL is ready
        if (game == nullptr) {
            // Load OpenGL ES 3.x functions via EGL
            if (!gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress)) {
                LOGE("Failed to initialize GLAD for GLES. Ensure glad is built for ES3.");
            } else {
                LOGI("GLAD initialized for OpenGL ES");
            }

            // 2. Instantiate the Game and start the Lua script
            game = std::make_unique<Game>();
            try {
                game->start();
                LOGI("Game started successfully");
            } catch (const std::exception& e) {
                LOGE("Failed to start game: %s", e.what());
                game.reset();
            }
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 3. Run the engine update loop (Lua, Animator, Physics)
        if (game) {
            game->update(dt);
        }

        // ----------------------------------------------------------
        // RENDERING
        // ----------------------------------------------------------
        // TODO: Hook up XRManager for Quest VR rendering.
        // XRManager currently uses XrGraphicsBindingOpenGLWin32KHR.
        // It needs an #ifdef CHISEL_TARGET_ANDROID block to use 
        // XrGraphicsBindingOpenGLESAndroidKHR and EGL surfaces.
        //
        // For now, just clear the screen to prove the loop is running.
        
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // If you had desktop rendering adapted for GLES:
        // RenderSystem::getInstance().render(game->getSceneRoot());

        eglSwapBuffers(g_state.display, g_state.surface);
    }

cleanup:
    // 4. Clean shutdown
    if (game) {
        game.reset();
    }
    
    PhysicsSystem::getInstance().shutdown();
    // RenderSystem::getInstance().shutdown(); 
    // XRManager::getInstance().shutdown();
    
    shutdownEGL();
    LOGI("ChiselEngine exited cleanly");
}