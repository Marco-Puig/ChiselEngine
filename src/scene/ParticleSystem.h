#pragma once

#include "scene/Node.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <random>
#include <string>
#include <vector>

class Shader;

class ParticleSystem : public Node {
public:
    enum class BlendMode {
        Alpha,
        Additive
    };

    ParticleSystem(const std::string& name, int maxParticles = 512);
    ~ParticleSystem() override;

    void update(float deltaTime);
    void render(const glm::mat4& view, const glm::mat4& projection);

    void play() { m_playing = true; }
    void stop() { m_playing = false; }
    bool isPlaying() const { return m_playing; }

    void clear();

    void setEmissionRate(float particlesPerSecond) {
        m_emissionRate = std::max(0.0f, particlesPerSecond);
    }

    void setLifetime(float minLifetime, float maxLifetime) {
        m_minLifetime = std::max(0.01f, minLifetime);
        m_maxLifetime = std::max(m_minLifetime, maxLifetime);
    }

    void setSpeed(float minSpeed, float maxSpeed) {
        m_minSpeed = std::max(0.0f, minSpeed);
        m_maxSpeed = std::max(m_minSpeed, maxSpeed);
    }

    void setSpread(float radians) {
        m_spread = std::max(0.0f, radians);
    }

    void setSize(float startSize, float endSize) {
        m_startSize = std::max(0.001f, startSize);
        m_endSize = std::max(0.001f, endSize);
    }

    void setSpawnRadius(float radius) {
        m_spawnRadius = std::max(0.0f, radius);
    }

    void setGravity(const glm::vec3& acceleration) {
        m_gravity = acceleration;
    }

    void setBaseDirection(const glm::vec3& direction) {
        m_baseDirection = direction;
    }

protected:
    struct Particle {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};

        glm::vec4 startColor{1.0f};
        glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

        float age = 0.0f;
        float lifetime = 1.0f;

        float startSize = 0.1f;
        float endSize = 0.2f;

        unsigned int seed = 0;
    };

    virtual void initializeParticle(
        Particle& particle,
        const glm::mat4& worldTransform
    );

    virtual void updateParticle(Particle& particle, float deltaTime);

    virtual BlendMode getBlendMode() const {
        return BlendMode::Alpha;
    }

    static float randomRange(float minValue, float maxValue);
    static glm::vec3 randomInSphere(float radius);
    static glm::vec3 randomConeDirection(
        const glm::vec3& baseDirection,
        float spreadRadians
    );

private:
    void initGraphics();
    void emitParticles(int count, const glm::mat4& worldTransform);

    std::vector<Particle> m_particles;
    std::vector<float> m_vertexData;

    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    bool m_graphicsInitialized = false;

    int m_maxParticles = 512;

    bool m_playing = true;

    float m_emissionRate = 50.0f;
    float m_emitAccumulator = 0.0f;

    float m_minLifetime = 0.5f;
    float m_maxLifetime = 1.5f;

    float m_minSpeed = 0.5f;
    float m_maxSpeed = 1.5f;

    float m_spread = 0.35f;

    float m_startSize = 0.1f;
    float m_endSize = 0.3f;

    float m_spawnRadius = 0.05f;

    glm::vec3 m_baseDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 m_gravity{0.0f, 0.0f, 0.0f};

    static std::mt19937& randomGenerator();
};


void updateParticleSystems(Node* root, float deltaTime);
void renderParticleSystems(
    Node* root,
    const glm::mat4& view,
    const glm::mat4& projection
);