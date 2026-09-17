#pragma once

class Node;

class IGame {
public:
    virtual ~IGame() = default;
    virtual void start() = 0;
    virtual void update(float deltaTime) = 0;
    virtual Node* getSceneRoot() const = 0;
};
