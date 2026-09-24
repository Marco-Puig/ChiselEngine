#pragma once

#include <memory>
#include <string>

class AudioSystem {
public:
    static AudioSystem& getInstance();

    bool init();
    void shutdown();
    void update();

    bool playSound(
        const std::string& path,
        bool loop = false,
        float volume = 1.0f
    );

    bool playMusic(
        const std::string& path,
        bool loop = true,
        float volume = 0.8f
    );

    void stopMusic();
    void stopAllSounds();
    void stopAll();

    void setMasterVolume(float volume);
    bool isInitialized() const;

private:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};