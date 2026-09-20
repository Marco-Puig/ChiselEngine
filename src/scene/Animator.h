#pragma once
#include "Node.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

enum class Axis { X, Y, Z };
enum class Direction { Positive = 1, Negative = -1 };
enum class TransformType { Rotation, Position };

class Animator {
public:
    struct KeyframeVec3 {
        float time;
        glm::vec3 value;
    };

    struct KeyframeQuat {
        float time;
        glm::quat value;
    };

    struct AnimationChannel {
        Node* targetNode;
        std::string path; // "translation", "rotation", "scale"
        std::vector<KeyframeVec3> vec3Keys;
        std::vector<KeyframeQuat> quatKeys;
    };

    struct AnimationClip {
        std::string name;
        float duration = 0.0f;
        std::vector<AnimationChannel> channels;
    };

    struct AnimationInfo {
        Node* node;
        std::string label;
    };

    explicit Animator(Node* target = nullptr) : m_target(target) {}

    int procedural(Node* node, Axis axis, Direction direction,
                    TransformType type, float speed) {
        if (node == nullptr || speed < 0.0f)
            return -1;
        node->setAnimationDriven(true);
        m_procedures.push_back({node, axis, direction, type, speed,
                                makeLabel(axis, direction, type, speed), true});
        return static_cast<int>(m_procedures.size() - 1);
    }

    void update(float deltaTime) {
        // 1. Update Procedural Animations
        for (const Procedure& procedure : m_procedures) {
            if (!procedure.active) continue;

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
        
        // 2. Update GLB Node Animations
        for (auto& activeAnim : m_activeNodeAnimations) {
            if (!activeAnim.active) continue;
            
            activeAnim.currentTime += deltaTime;
            if (activeAnim.currentTime > activeAnim.clip.duration && activeAnim.clip.duration > 0.0f) {
                activeAnim.currentTime = fmod(activeAnim.currentTime, activeAnim.clip.duration); // Loop
            }

            for (const auto& channel : activeAnim.clip.channels) {
                if (!channel.targetNode) continue;
                channel.targetNode->setAnimationDriven(true);

                if (channel.path == "translation" && !channel.vec3Keys.empty()) {
                    glm::vec3 val = evaluateVec3(channel.vec3Keys, activeAnim.currentTime);
                    channel.targetNode->setPosition(val);
                } else if (channel.path == "rotation" && !channel.quatKeys.empty()) {
                    glm::quat val = evaluateQuat(channel.quatKeys, activeAnim.currentTime);
                    channel.targetNode->setRotation(val);
                } else if (channel.path == "scale" && !channel.vec3Keys.empty()) {
                    glm::vec3 val = evaluateVec3(channel.vec3Keys, activeAnim.currentTime);
                    channel.targetNode->setScale(val);
                }
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

    int proceduralFromLua(Node* node, const std::string& axis,
                           const std::string& direction,
                           const std::string& type, float speed) {
        Axis selectedAxis = Axis::Y;
        if (axis == "x") selectedAxis = Axis::X;
        else if (axis == "z") selectedAxis = Axis::Z;

        const Direction selectedDirection =
            direction == "negative" ? Direction::Negative : Direction::Positive;
        const TransformType selectedType =
            type == "position" ? TransformType::Position : TransformType::Rotation;
        
        return procedural(node, selectedAxis, selectedDirection, selectedType, speed);
    }

    // --- Procedural Play/Stop Controls ---
    void playProcedural(int id) {
        if (id >= 0 && id < static_cast<int>(m_procedures.size())) 
            m_procedures[id].active = true;
    }

    void stopProcedural(int id) {
        if (id >= 0 && id < static_cast<int>(m_procedures.size())) 
            m_procedures[id].active = false;
    }

    // --- GLB Node Animation Controls ---
    void registerAnimationClip(const std::string& meshKey, const AnimationClip& clip) {
        m_animationLibrary[meshKey][clip.name] = clip;
    }

    void playAnimation(Node* node, const std::string& animName) {
        if (!node) return;
        
        // Search loaded animation library for clip matching animName
        for (auto& [meshKey, clips] : m_animationLibrary) {
            if (clips.find(animName) != clips.end()) {
                // Check if already active
                for (auto& active : m_activeNodeAnimations) {
                    if (active.clip.name == animName) {
                        active.active = true;
                        return;
                    }
                }
                // Add new active animation instance
                m_activeNodeAnimations.push_back({clips[animName], 0.0f, true});
                return;
            }
        }
    }

    void stopAnimation(Node* node, const std::string& animName) {
        if (!node) return;
        for (auto& anim : m_activeNodeAnimations) {
            if (anim.clip.name == animName) {
                anim.active = false;
            }
        }
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

    glm::vec3 evaluateVec3(const std::vector<KeyframeVec3>& keys, float time) {
        if (keys.empty()) return glm::vec3(0.0f);
        if (time <= keys.front().time) return keys.front().value;
        if (time >= keys.back().time) return keys.back().value;

        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (time >= keys[i].time && time <= keys[i + 1].time) {
                float t0 = keys[i].time;
                float t1 = keys[i + 1].time;
                float factor = (t1 - t0 > 0.0f) ? (time - t0) / (t1 - t0) : 0.0f;
                return glm::mix(keys[i].value, keys[i + 1].value, factor);
            }
        }
        return keys.back().value;
    }

    glm::quat evaluateQuat(const std::vector<KeyframeQuat>& keys, float time) {
        if (keys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        if (time <= keys.front().time) return keys.front().value;
        if (time >= keys.back().time) return keys.back().value;

        for (size_t i = 0; i < keys.size() - 1; ++i) {
            if (time >= keys[i].time && time <= keys[i + 1].time) {
                float t0 = keys[i].time;
                float t1 = keys[i + 1].time;
                float factor = (t1 - t0 > 0.0f) ? (time - t0) / (t1 - t0) : 0.0f;
                return glm::slerp(keys[i].value, keys[i + 1].value, factor);
            }
        }
        return keys.back().value;
    }

    struct Procedure {
        Node* node;
        Axis axis;
        Direction direction;
        TransformType type;
        float speed;
        std::string label;
        bool active;
    };

    struct ActiveAnimation {
        AnimationClip clip;
        float currentTime = 0.0f;
        bool active = false;
    };

    Node* m_target = nullptr;
    std::vector<Procedure> m_procedures;
    std::unordered_map<std::string, std::unordered_map<std::string, AnimationClip>> m_animationLibrary;
    std::vector<ActiveAnimation> m_activeNodeAnimations;
};