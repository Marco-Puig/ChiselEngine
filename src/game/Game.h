#pragma once
#include "core/IGame.h"
#include "scene/Node.h"
#include "scene/Animator.h"
#include <memory>

class Game : public IGame {
public:
    Game();
    void start() override;
    void update(float deltaTime) override;
    Node* getSceneRoot() const { return sceneRoot.get(); }

private:
    std::unique_ptr<Node> sceneRoot;
    std::unique_ptr<Animator> animator;
    Node* cube = nullptr;
};
