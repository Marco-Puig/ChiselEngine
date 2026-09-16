#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

struct Keyframe {
    float time;
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
};

struct AnimationTrack {
    std::string nodeName;
    std::vector<Keyframe> keyframes;
};

struct AnimationClip {
    std::string name;
    float duration;
    std::vector<AnimationTrack> tracks;
};

class AnimationLibrary {
public:
    static AnimationLibrary& getInstance() {
        static AnimationLibrary instance;
        return instance;
    }

    void addClip(const AnimationClip& clip) {
        m_clips[clip.name] = clip;
    }

    const AnimationClip* getClip(const std::string& name) const {
        auto it = m_clips.find(name);
        return (it != m_clips.end()) ? &it->second : nullptr;
    }

private:
    AnimationLibrary() = default;
    std::map<std::string, AnimationClip> m_clips;
};
