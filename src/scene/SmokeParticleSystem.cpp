#include "scene/SmokeParticleSystem.h"

#include <cmath>

SmokeParticleSystem::SmokeParticleSystem(const std::string& name)
    : ParticleSystem(name, 450) {
    setEmissionRate(35.0f);
    setLifetime(1.8f, 4.0f);
    setSpeed(0.25f, 0.85f);
    setSpread(0.6f);
    setSize(0.25f, 1.0f);
    setSpawnRadius(0.12f);
    setGravity(glm::vec3(0.0f, 0.15f, 0.0f));
    setBaseDirection(glm::vec3(0.0f, 1.0f, 0.0f));
}

void SmokeParticleSystem::initializeParticle(
    Particle& particle,
    const glm::mat4& worldTransform
) {
    ParticleSystem::initializeParticle(particle, worldTransform);

    const float shade = randomRange(0.25f, 0.55f);

    particle.startColor = glm::vec4(
        shade,
        shade,
        shade,
        randomRange(0.12f, 0.22f)
    );

    particle.endColor = glm::vec4(
        shade * 1.3f,
        shade * 1.3f,
        shade * 1.3f,
        0.0f
    );

    particle.startSize *= randomRange(0.8f, 1.3f);
    particle.endSize *= randomRange(1.0f, 1.6f);
}

void SmokeParticleSystem::updateParticle(
    Particle& particle,
    float deltaTime
) {
    ParticleSystem::updateParticle(particle, deltaTime);

    const float phase =
        static_cast<float>(particle.seed % 1000) * 0.01f +
        particle.age * 1.5f;

    particle.velocity.x += std::sin(phase) * 0.05f * deltaTime;
    particle.velocity.z += std::cos(phase * 0.91f) * 0.05f * deltaTime;
}