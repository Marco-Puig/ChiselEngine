#pragma once
#include "core/IGame.h"
#include "scene/Scene.h"
#include "scene/Animator.h"
#include "scene/ArcRotateCamera.h"
#include "scripting/LuaRuntime.h"
#include <memory>

class Game : public IGame {
public:
    Game();
    void start() override;
    void update(float deltaTime) override;
    Node* getSceneRoot() const { return scene->getRoot(); }
    ArcRotateCamera* getCamera() const override { return camera.get(); }

private:
    std::unique_ptr<Scene> scene;
    std::unique_ptr<Animator> animator;
    std::unique_ptr<LuaRuntime> luaRuntime;
    std::unique_ptr<ArcRotateCamera> camera;
};
