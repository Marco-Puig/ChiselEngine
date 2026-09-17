#pragma once
#include "Node.h"
#include <string>

class Animator {
public:
    Animator(Node* target) : m_target(target) {}

    void animateAxis(const std::string& axis, float amount) {
        glm::vec3 pos = m_target->getPosition();
        if (axis == "x") pos.x += amount;
        else if (axis == "y") pos.y += amount;
        else if (axis == "z") pos.z += amount;
        m_target->setPosition(pos);
    }

    void playAnimation(const std::string& animName) {
        // GLB animation trigger logic will be integrated with GLBLoader in Step 4
    }

private:
    Node* m_target;
};
