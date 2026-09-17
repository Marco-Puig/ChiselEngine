#pragma once

class IGame {
public:
    virtual ~IGame() = default;
    virtual void start() = 0;
    virtual void update(float deltaTime) = 0;
};
