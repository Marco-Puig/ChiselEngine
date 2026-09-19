#pragma once

class Node;
class ArcRotateCamera;
class Animator;

class IGame {
public:
    virtual ~IGame() = default;
    virtual void start() = 0;
    virtual void update(float deltaTime) = 0;
    virtual Node* getSceneRoot() const = 0;
    virtual ArcRotateCamera* getCamera() const { return nullptr; }
    virtual Animator* getAnimator() const { return nullptr; }
};
