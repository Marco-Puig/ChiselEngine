#pragma once

#include "scene/ParticleSystem.h"

class FireParticleSystem : public ParticleSystem {
public:
    explicit FireParticleSystem(const std::string& name);

protected:
    void initializeParticle(
        Particle& particle,
        const glm::mat4& worldTransform
    ) override;

    void updateParticle(Particle& particle, float deltaTime) override;

    BlendMode getBlendMode() const override {
        return BlendMode::Additive;
    }
};