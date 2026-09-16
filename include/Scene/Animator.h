#pragma once
#include "Node.h"
#include "AnimationLibrary.h"
#include <functional>
#include <string>
#include <map>


enum class AnimationType {
    Procedural,
    Keyframe
};

struct AnimationClip {
    std::string name;
    float duration;
    // In a full implementation, this would hold a map of Node names to keyframe tracks
};

class Animator {
public:
    Animator(Node* target) : m_target(target) {}

    // Procedural Animation: Move target along an axis by a value per second
    void animateAxis(const std::string& axis, float valuePerSecond);
    
    // Keyframe Animation: Play an animation associated with the node (e.g. from GLB)
    void playAnimation(const std::string& clipName);

    void update(float deltaTime);

private:
    Node* m_target;
    
    // Procedural state
    glm::vec3 m_procVelocity = glm::vec3(0.0f);
    
    // Animation state
    std::string m_currentClip;
    float m_currentTime = 0.0f;
    bool m_isPlaying = false;
};
