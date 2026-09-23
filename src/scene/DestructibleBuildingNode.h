#pragma once

#include "scene/MeshNode.h"
#include "rendering/Material.h"

#include <glm/glm.hpp>

class Scene;

class DestructibleBuildingNode : public MeshNode {
public:
    DestructibleBuildingNode(
        const std::string& name,
        Scene* scene,
        const glm::vec3& size,
        float health
    );

    void applyDamage(float amount);
    void destroy();

    float getHealth() const { return m_health; }
    float getMaxHealth() const { return m_maxHealth; }
    bool isDestroyed() const { return m_destroyed; }

private:
    void refreshAppearance();
    void spawnDebris(const glm::mat4& cachedWorldTransform);

    Scene* m_scene = nullptr;

    glm::vec3 m_size{1.0f, 3.0f, 1.0f};

    float m_maxHealth = 100.0f;
    float m_health = 100.0f;

    bool m_destroyed = false;

    Material m_material;
};