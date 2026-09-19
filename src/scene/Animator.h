#pragma once
#include "Node.h"
#include <string>
#include <vector>

enum class Axis { X, Y, Z };
enum class Direction { Positive = 1, Negative = -1 };
enum class TransformType { Rotation, Position };

class Animator {
public:
    struct AnimationInfo {
        Node* node;
        std::string label;
    };
    explicit Animator(Node* target = nullptr) : m_target(target) {}

    void procedural(Node* node, Axis axis, Direction direction,
                    TransformType type, float speed) {
        if (node == nullptr || speed < 0.0f)
            return;
        node->setAnimationDriven(true);
        m_procedures.push_back({node, axis, direction, type, speed,
                                makeLabel(axis, direction, type, speed)});
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

    void proceduralFromLua(Node* node, const std::string& axis,
                           const std::string& direction,
                           const std::string& type, float speed) {
        Axis selectedAxis = Axis::Y;
        if (axis == "x") selectedAxis = Axis::X;
        else if (axis == "z") selectedAxis = Axis::Z;

        const Direction selectedDirection =
            direction == "negative" ? Direction::Negative : Direction::Positive;
        const TransformType selectedType =
            type == "position" ? TransformType::Position : TransformType::Rotation;
        procedural(node, selectedAxis, selectedDirection, selectedType, speed);
    }

    void playAnimation(const std::string& animName) {
        (void)animName;
    }

    std::vector<std::string> getAnimationLabels(const Node* node) const {
        std::vector<std::string> result;
        for (const Procedure& procedure : m_procedures) {
            if (procedure.node != node)
                continue;
            result.push_back(procedure.label);
        }
        return result;
    }

private:
    static std::string makeLabel(Axis axis, Direction direction,
                                 TransformType type, float speed) {
        const char* axisName = axis == Axis::X ? "X" :
                               axis == Axis::Y ? "Y" : "Z";
        const char* typeName = type == TransformType::Rotation ?
                               "Rotation" : "Position";
        const char* directionName = direction == Direction::Positive ?
                                     "+" : "-";
        return std::string(typeName) + " " + axisName + " (" +
               directionName + ", " + std::to_string(speed) + "/s)";
    }

    struct Procedure {
        Node* node;
        Axis axis;
        Direction direction;
        TransformType type;
        float speed;
        std::string label;
    };

    Node* m_target = nullptr;
    std::vector<Procedure> m_procedures;
};
