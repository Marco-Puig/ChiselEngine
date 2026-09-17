#pragma once
#include "Node.h"
#include <string>
#include <vector>

enum class Axis { X, Y, Z };
enum class Direction { Positive = 1, Negative = -1 };
enum class TransformType { Rotation, Position };

class Animator {
public:
    explicit Animator(Node* target = nullptr) : m_target(target) {}

    void procedural(Node* node, Axis axis, Direction direction,
                    TransformType type, float speed) {
        if (node == nullptr || speed < 0.0f)
            return;
        m_procedures.push_back({node, axis, direction, type, speed});
    }

    void update(float deltaTime) {
        for (const Procedure& procedure : m_procedures) {
            const float amount = procedure.speed * deltaTime *
                                 static_cast<float>(procedure.direction);
            glm::vec3 axis(0.0f);
            if (procedure.axis == Axis::X) axis.x = 1.0f;
            if (procedure.axis == Axis::Y) axis.y = 1.0f;
            if (procedure.axis == Axis::Z) axis.z = 1.0f;

            if (procedure.type == TransformType::Position) {
                procedure.node->setPosition(procedure.node->getPosition() + axis * amount);
            } else {
                const glm::quat delta = glm::angleAxis(amount, axis);
                procedure.node->setRotation(glm::normalize(delta * procedure.node->getRotation()));
            }
        }
    }

    void animateAxis(const std::string& axisName, float amount) {
        if (m_target == nullptr)
            return;
        glm::vec3 position = m_target->getPosition();
        if (axisName == "x") position.x += amount;
        else if (axisName == "y") position.y += amount;
        else if (axisName == "z") position.z += amount;
        m_target->setPosition(position);
    }

    void playAnimation(const std::string& animName) {
        (void)animName;
    }

private:
    struct Procedure {
        Node* node;
        Axis axis;
        Direction direction;
        TransformType type;
        float speed;
    };

    Node* m_target = nullptr;
    std::vector<Procedure> m_procedures;
};
