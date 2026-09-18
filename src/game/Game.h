#pragma once
#include "core/IGame.h"
#include "scene/Node.h"
#include "scene/Animator.h"
#include "scene/ArcRotateCamera.h"
#include <memory>

class Game : public IGame {
public:
    Game();
    void start() override;
    void update(float deltaTime) override;
    Node* getSceneRoot() const { return sceneRoot.get(); }
    ArcRotateCamera* getCamera() const override { return camera.get(); }

private:
    std::unique_ptr<Node> sceneRoot;
    std::unique_ptr<Animator> animator;
    Node* cube = nullptr;
    Node* plane = nullptr;
    std::unique_ptr<ArcRotateCamera> camera;
};
