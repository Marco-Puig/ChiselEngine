#pragma once
#include "core/IGame.h"
#include "scene/Node.h"
#include <memory>

class Game : public IGame {
public:
    Game();
    void start() override;
    void update(float deltaTime) override;
    Node* getSceneRoot() const { return sceneRoot.get(); }

private:
    std::unique_ptr<Node> sceneRoot;
    class Animator* swordAnimator = nullptr;
};
