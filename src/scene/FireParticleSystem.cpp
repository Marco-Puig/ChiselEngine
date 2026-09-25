#include "scene/FireParticleSystem.h"

#include <cmath>

FireParticleSystem::FireParticleSystem(const std::string& name)
    : ParticleSystem(name, 600) {
    setEmissionRate(120.0f);
    setLifetime(0.45f, 1.1f);
    setSpeed(0.8f, 2.2f);
    setSpread(0.35f);
    setSize(0.12f, 0.5f);
    setSpawnRadius(0.08f);
    setGravity(glm::vec3(0.0f, 1.2f, 0.0f));
    setBaseDirection(glm::vec3(0.0f, 1.0f, 0.0f));
}

void FireParticleSystem::initializeParticle(
    Particle& particle,
    const glm::mat4& worldTransform
) {
    ParticleSystem::initializeParticle(particle, worldTransform);

    const float heat = randomRange(0.0f, 1.0f);

    const glm::vec4 yellow(1.0f, 0.9f, 0.35f, 1.0f);
    const glm::vec4 orange(1.0f, 0.45f, 0.1f, 1.0f);

    particle.startColor = glm::mix(yellow, orange, heat);
    particle.endColor = glm::vec4(0.15f, 0.02f, 0.0f, 0.0f);

    particle.startSize *= randomRange(0.8f, 1.2f);
    particle.endSize *= randomRange(0.9f, 1.4f);
}

void FireParticleSystem::updateParticle(
    Particle& particle,
    float deltaTime
) {
    ParticleSystem::updateParticle(particle, deltaTime);

    const float phase =
        static_cast<float>(particle.seed % 1000) * 0.01f +
        particle.age * 6.0f;

    particle.velocity.x += std::sin(phase) * 0.12f * deltaTime;
    particle.velocity.z += std::cos(phase * 0.87f) * 0.12f * deltaTime;
}