#include "Scene/Animator.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Scene/AnimationLibrary.h"

void Animator::animateAxis(const std::string& axis, float valuePerSecond) {
    if (axis == "x") m_procVelocity.x = valuePerSecond;
    else if (axis == "y") m_procVelocity.y = valuePerSecond;
    else if (axis == "z") m_procVelocity.z = valuePerSecond;
}

void Animator::playAnimation(const std::string& clipName) {
    m_currentClip = clipName;
    m_currentTime = 0.0f;
    m_isPlaying = true;
}

void Animator::update(float deltaTime) {
    if (!m_target) return;

    // 1. Procedural Animation Logic
    glm::vec3 currentPos = m_target->getPosition();
    m_target->setPosition(currentPos + (m_procVelocity * deltaTime));

    // 2. Keyframe Animation Logic
    if (m_isPlaying && !m_currentClip.empty()) {
        m_currentTime += deltaTime;
        
        const AnimationClip* clip = AnimationLibrary::getInstance().getClip(m_currentClip);
        if (clip) {
            for (const auto& track : clip->tracks) {
                if (track.nodeName == m_target->getName()) {
                    if (track.keyframes.size() < 2) continue;

                    size_t nextIdx = 0;
                    while (nextIdx < track.keyframes.size() && track.keyframes[nextIdx].time < m_currentTime) {
                        nextIdx++;
                    }

                    if (nextIdx == 0) {
                        m_target->setPosition(track.keyframes[0].position);
                        m_target->setRotation(track.keyframes[0].rotation);
                    } else if (nextIdx == track.keyframes.size()) {
                        m_currentTime = fmod(m_currentTime, clip->duration);
                        update(0); 
                        return;
                    } else {
                        const Keyframe& k1 = track.keyframes[nextIdx - 1];
                        const Keyframe& k2 = track.keyframes[nextIdx];
                        float t = (m_currentTime - k1.time) / (k2.time - k1.time);
                        
                        glm::vec3 pos = glm::mix(k1.position, k2.position, t);
                        glm::quat rot = glm::slerp(k1.rotation, k2.rotation, t);
                        
                        m_target->setPosition(pos);
                        m_target->setRotation(rot);
                    }
                    break;
                }
            }
        }
    }
}


