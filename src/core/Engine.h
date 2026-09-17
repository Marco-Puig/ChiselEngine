#pragma once
#include <memory>
#include "platform/Window.h"
#include "core/IGame.h"

class Engine {
public:
    static Engine& getInstance() {
        static Engine instance;
        return instance;
    }

    void init();
    void run(IGame* game);

private:
    Engine() = default;
    std::unique_ptr<Window> m_window;
};
