#include "Game.h"
#include <stdexcept>

Game::Game()
    : scene(std::make_unique<Scene>()),
      animator(std::make_unique<Animator>()),
      luaRuntime(std::make_unique<LuaRuntime>()),
      camera(std::make_unique<ArcRotateCamera>("DesktopCamera")) {}

void Game::start() {
    if (!luaRuntime->initialize())
        throw std::runtime_error("Failed to initialize Lua runtime");
    if (!luaRuntime->loadGameScript("Game/game.lua"))
        throw std::runtime_error("Failed to load game script");
    luaRuntime->startGame(*scene, *animator);
}

void Game::update(float deltaTime) {
    luaRuntime->updateGame(deltaTime);
    animator->update(deltaTime);
    camera->update(deltaTime);
}
