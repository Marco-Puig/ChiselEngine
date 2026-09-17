#include "core/Engine.h"
#include "game/Game.h"
#include <iostream>

int main() {
    try {
        Engine& engine = Engine::getInstance();
        engine.init();
        
        Game game;
        engine.run(&game);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
