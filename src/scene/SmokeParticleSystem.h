#pragma once

#include "scene/ParticleSystem.h"

class SmokeParticleSystem : public ParticleSystem {
public:
    explicit SmokeParticleSystem(const std::string& name);

protected:
    void initializeParticle(
        Particle& particle,
        const glm::mat4& worldTransform
    ) override;

    void updateParticle(Particle& particle, float deltaTime) override;

    BlendMode getBlendMode() const override {
        return BlendMode::Alpha;
    }
};