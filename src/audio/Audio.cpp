#include "Audio.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winmm.lib")
#endif

namespace {

float clampVolume(float volume) {
    if (volume < 0.0f) {
        return 0.0f;
    }

    if (volume > 1.0f) {
        return 1.0f;
    }

    return volume;
}

bool hasWavExtension(const std::string& path) {
    const std::string extension = ".wav";

    if (path.size() < extension.size()) {
        return false;
    }

    const size_t offset = path.size() - extension.size();

    for (size_t i = 0; i < extension.size(); ++i) {
        const char a = static_cast<char>(
            std::tolower(static_cast<unsigned char>(path[offset + i]))
        );

        const char b = extension[i];

        if (a != b) {
            return false;
        }
    }

    return true;
}

}

struct AudioSystem::Impl {
    struct ActiveSound {
        ma_sound sound{};
        bool initialized = false;
    };

    ma_engine engine{};
    bool engineInitialized = false;
    float masterVolume = 1.0f;

    std::mutex mutex;

    std::vector<std::unique_ptr<ActiveSound>> sounds;
    std::unique_ptr<ActiveSound> music;

    bool init() {
        if (engineInitialized) {
            return true;
        }

        ma_engine_config config = ma_engine_config_init();

        config.sampleRate = 0;

        const ma_result result = ma_engine_init(&config, &engine);

        if (result != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to initialize audio engine: "
                      << result << '\n';
            return false;
        }

        engineInitialized = true;

        ma_engine_set_volume(&engine, masterVolume);

        return true;
    }

    bool ensureInitialized() {
        if (engineInitialized) {
            return true;
        }

        return init();
    }

    void shutdown() {
        stopAll();

        if (engineInitialized) {
            ma_engine_uninit(&engine);
            engineInitialized = false;
        }
    }

    void update() {
        if (!engineInitialized) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex);

        cleanupFinishedMusicLocked();
        cleanupFinishedSoundsLocked();
    }

    void setMasterVolume(float volume) {
        masterVolume = clampVolume(volume);

        if (engineInitialized) {
            ma_engine_set_volume(&engine, masterVolume);
        }
    }

    bool playSound(const std::string& path, bool loop, float volume) {
        return playInternal(path, loop, volume, false);
    }

    bool playMusic(const std::string& path, bool loop, float volume) {
        return playInternal(path, loop, volume, true);
    }

    void stopMusic() {
        std::lock_guard<std::mutex> lock(mutex);
        stopMusicLocked();
    }

    void stopAllSounds() {
        std::lock_guard<std::mutex> lock(mutex);
        stopSoundsLocked();
    }

    void stopAll() {
        std::lock_guard<std::mutex> lock(mutex);
        stopMusicLocked();
        stopSoundsLocked();
    }

private:
    bool playInternal(
        const std::string& path,
        bool loop,
        float volume,
        bool asMusic
    ) {
        if (!ensureInitialized()) {
            return false;
        }

        if (!hasWavExtension(path)) {
            std::cerr << "[Audio] Warning: expected a .wav file: "
                      << path << '\n';
        }

        auto entry = std::make_unique<ActiveSound>();

        const ma_result initResult = ma_sound_init_from_file(
            &engine,
            path.c_str(),
            0,
            nullptr,
            nullptr,
            &entry->sound
        );

        if (initResult != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to open audio file: "
                      << path
                      << " (error " << initResult << ")\n";
            return false;
        }

        entry->initialized = true;

        ma_sound_set_looping(
            &entry->sound,
            loop ? MA_TRUE : MA_FALSE
        );

        ma_sound_set_volume(
            &entry->sound,
            clampVolume(volume)
        );

        const ma_result startResult = ma_sound_start(&entry->sound);

        if (startResult != MA_SUCCESS) {
            std::cerr << "[Audio] Failed to start audio file: "
                      << path
                      << " (error " << startResult << ")\n";

            ma_sound_uninit(&entry->sound);
            entry->initialized = false;

            return false;
        }

        std::lock_guard<std::mutex> lock(mutex);

        if (asMusic) {
            stopMusicLocked();
            music = std::move(entry);
        } else {
            sounds.push_back(std::move(entry));
        }

        return true;
    }

    void cleanupFinishedSoundsLocked() {
        sounds.erase(
            std::remove_if(
                sounds.begin(),
                sounds.end(),
                [](const std::unique_ptr<ActiveSound>& sound) {
                    if (!sound || !sound->initialized) {
                        return true;
                    }

                    if (ma_sound_at_end(&sound->sound)) {
                        ma_sound_uninit(&sound->sound);
                        sound->initialized = false;
                        return true;
                    }

                    return false;
                }
            ),
            sounds.end()
        );
    }

    void cleanupFinishedMusicLocked() {
        if (!music || !music->initialized) {
            music.reset();
            return;
        }

        if (ma_sound_at_end(&music->sound)) {
            ma_sound_uninit(&music->sound);
            music->initialized = false;
            music.reset();
        }
    }

    void stopSoundsLocked() {
        for (auto& sound : sounds) {
            if (sound && sound->initialized) {
                ma_sound_stop(&sound->sound);
                ma_sound_uninit(&sound->sound);
                sound->initialized = false;
            }
        }

        sounds.clear();
    }

    void stopMusicLocked() {
        if (music && music->initialized) {
            ma_sound_stop(&music->sound);
            ma_sound_uninit(&music->sound);
            music->initialized = false;
        }

        music.reset();
    }
};

AudioSystem& AudioSystem::getInstance() {
    static AudioSystem instance;
    return instance;
}

AudioSystem::AudioSystem()
    : m_impl(std::make_unique<Impl>()) {}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    return m_impl->init();
}

void AudioSystem::shutdown() {
    m_impl->shutdown();
}

void AudioSystem::update() {
    m_impl->update();
}

bool AudioSystem::playSound(
    const std::string& path,
    bool loop,
    float volume
) {
    return m_impl->playSound(path, loop, volume);
}

bool AudioSystem::playMusic(
    const std::string& path,
    bool loop,
    float volume
) {
    return m_impl->playMusic(path, loop, volume);
}

void AudioSystem::stopMusic() {
    m_impl->stopMusic();
}

void AudioSystem::stopAllSounds() {
    m_impl->stopAllSounds();
}

void AudioSystem::stopAll() {
    m_impl->stopAll();
}

void AudioSystem::setMasterVolume(float volume) {
    m_impl->setMasterVolume(volume);
}

bool AudioSystem::isInitialized() const {
    return m_impl->engineInitialized;
}