#include "scene/DestructibleBuildingNode.h"
#include "scene/Scene.h"
#include "platform/PhysicsSystem.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace {

struct BoxMeshData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

BoxMeshData buildBoxMesh(const glm::vec3& min, const glm::vec3& max) {
    BoxMeshData out;

    auto mapPoint = [&](const glm::vec3& unit) {
        const glm::vec3 t = unit * 0.5f + glm::vec3(0.5f);
        return min + t * (max - min);
    };

    struct UnitFace {
        glm::vec3 normal;
        glm::vec3 p[4];
    };

    const UnitFace faces[] = {
        {
            {1.0f, 0.0f, 0.0f},
            {
                {1.0f, -1.0f, 1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f, 1.0f, -1.0f},
                {1.0f, 1.0f, 1.0f}
            }
        },
        {
            {-1.0f, 0.0f, 0.0f},
            {
                {-1.0f, -1.0f, -1.0f},
                {-1.0f, -1.0f, 1.0f},
                {-1.0f, 1.0f, 1.0f},
                {-1.0f, 1.0f, -1.0f}
            }
        },
        {
            {0.0f, 1.0f, 0.0f},
            {
                {-1.0f, 1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f},
                {1.0f, 1.0f, -1.0f},
                {-1.0f, 1.0f, -1.0f}
            }
        },
        {
            {0.0f, -1.0f, 0.0f},
            {
                {-1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, -1.0f},
                {1.0f, -1.0f, 1.0f},
                {-1.0f, -1.0f, 1.0f}
            }
        },
        {
            {0.0f, 0.0f, 1.0f},
            {
                {-1.0f, -1.0f, 1.0f},
                {1.0f, -1.0f, 1.0f},
                {1.0f, 1.0f, 1.0f},
                {-1.0f, 1.0f, 1.0f}
            }
        },
        {
            {0.0f, 0.0f, -1.0f},
            {
                {1.0f, -1.0f, -1.0f},
                {-1.0f, -1.0f, -1.0f},
                {-1.0f, 1.0f, -1.0f},
                {1.0f, 1.0f, -1.0f}
            }
        }
    };

    for (const auto& face : faces) {
        const unsigned int base =
            static_cast<unsigned int>(out.vertices.size() / 8);

        const glm::vec2 uvs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        for (int i = 0; i < 4; ++i) {
            const glm::vec3 position = mapPoint(face.p[i]);

            out.vertices.push_back(position.x);
            out.vertices.push_back(position.y);
            out.vertices.push_back(position.z);

            out.vertices.push_back(face.normal.x);
            out.vertices.push_back(face.normal.y);
            out.vertices.push_back(face.normal.z);

            out.vertices.push_back(uvs[i].x);
            out.vertices.push_back(uvs[i].y);
        }

        out.indices.insert(
            out.indices.end(),
            {
                base,
                base + 1,
                base + 2,
                base,
                base + 2,
                base + 3
            }
        );
    }

    return out;
}

void setupBoxMesh(
    MeshNode* node,
    const glm::vec3& min,
    const glm::vec3& max
) {
    if (node == nullptr) {
        return;
    }

    const BoxMeshData mesh = buildBoxMesh(min, max);

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        mesh.vertices.size() * sizeof(float),
        mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mesh.indices.size() * sizeof(unsigned int),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    node->setMesh(
        vao,
        vbo,
        ebo,
        static_cast<int>(mesh.indices.size()),
        true
    );

    node->setBounds(min, max);

    std::vector<glm::vec3> collisionVertices = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z}
    };

    node->setCollisionVertices(std::move(collisionVertices));
}

}

DestructibleBuildingNode::DestructibleBuildingNode(
    const std::string& name,
    Scene* scene,
    const glm::vec3& size,
    float health
)
    : MeshNode(name),
      m_scene(scene) {
    m_size = glm::max(size, glm::vec3(0.01f));
    m_maxHealth = glm::max(1.0f, health);
    m_health = m_maxHealth;

    const glm::vec3 min(
        -m_size.x * 0.5f,
        0.0f,
        -m_size.z * 0.5f
    );

    const glm::vec3 max(
        m_size.x * 0.5f,
        m_size.y,
        m_size.z * 0.5f
    );

    setupBoxMesh(this, min, max);

    m_material = Material();
    m_material.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_material.metallicFactor = 0.0f;
    m_material.roughnessFactor = 0.85f;
    m_material.emissiveFactor = glm::vec3(0.0f);

    setMaterial(m_material);
}

void DestructibleBuildingNode::applyDamage(float amount) {
    if (m_destroyed || amount <= 0.0f) {
        return;
    }

    m_health -= amount;

    if (m_health <= 0.0f) {
        m_health = 0.0f;
        destroy();
        return;
    }

    refreshAppearance();
}

void DestructibleBuildingNode::destroy() {
    if (m_destroyed) {
        return;
    }

    m_destroyed = true;

    const glm::mat4 cachedWorldTransform = getWorldTransform();
    spawnDebris(cachedWorldTransform);
    setScale(glm::vec3(0.0001f));
    PhysicsSystem::getInstance().setBodyPosition(
        this,
        glm::vec3(0.0f, -100000.0f, 0.0f)
    );
}

void DestructibleBuildingNode::refreshAppearance() {
    if (m_destroyed) {
        return;
    }

    const float health01 = glm::clamp(m_health / m_maxHealth, 0.0f, 1.0f);

    const glm::vec3 white(1.0f);
    const glm::vec3 damaged(0.35f, 0.32f, 0.30f);

    const glm::vec3 color = glm::mix(damaged, white, health01);

    m_material.baseColorFactor = glm::vec4(color, 1.0f);
    setMaterial(m_material);
}

void DestructibleBuildingNode::spawnDebris(const glm::mat4& cachedWorldTransform) {
    if (m_scene == nullptr) {
        return;
    }

    const glm::quat buildingRotation = glm::quat_cast(cachedWorldTransform);

    const glm::vec3 buildingCenter = glm::vec3(
        cachedWorldTransform * glm::vec4(0.0f, m_size.y * 0.5f, 0.0f, 1.0f)
    );

    const int xSegments = 2;
    const int ySegments = 4;
    const int zSegments = 2;

    const float dx = m_size.x / static_cast<float>(xSegments);
    const float dy = m_size.y / static_cast<float>(ySegments);
    const float dz = m_size.z / static_cast<float>(zSegments);

    static std::mt19937 rng{std::random_device{}()};

    std::uniform_real_distribution<float> jitterDist(-0.2f, 0.2f);
    std::uniform_real_distribution<float> upDist(1.0f, 3.0f);
    std::uniform_real_distribution<float> speedDist(1.5f, 3.5f);

    int chunkIndex = 0;

    for (int x = 0; x < xSegments; ++x) {
        for (int y = 0; y < ySegments; ++y) {
            for (int z = 0; z < zSegments; ++z) {
                const glm::vec3 subMin(
                    -m_size.x * 0.5f + static_cast<float>(x) * dx,
                    static_cast<float>(y) * dy,
                    -m_size.z * 0.5f + static_cast<float>(z) * dz
                );

                const glm::vec3 subMax(
                    subMin.x + dx,
                    subMin.y + dy,
                    subMin.z + dz
                );

                const glm::vec3 chunkSize =
                    glm::max((subMax - subMin) * 0.94f, glm::vec3(0.01f));

                const glm::vec3 centerLocal = (subMin + subMax) * 0.5f;

                const glm::vec3 centerWorld = glm::vec3(
                    cachedWorldTransform * glm::vec4(centerLocal, 1.0f)
                );

                auto chunk = std::make_unique<MeshNode>(
                    getName() + "_chunk_" + std::to_string(chunkIndex)
                );

                setupBoxMesh(
                    chunk.get(),
                    -chunkSize * 0.5f,
                    chunkSize * 0.5f
                );

                chunk->setPosition(centerWorld);
                chunk->setRotation(buildingRotation);

                Material chunkMaterial = m_material;

                glm::vec3 color = glm::vec3(chunkMaterial.baseColorFactor);
                color += glm::vec3(jitterDist(rng) * 0.05f);
                color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));

                chunkMaterial.baseColorFactor = glm::vec4(color, 1.0f);
                chunk->setMaterial(chunkMaterial);

                MeshNode* chunkPtr = chunk.get();

                (void)m_scene->adopt(std::move(chunk));

                const glm::vec3 physicsSize = glm::max(chunkSize, glm::vec3(0.01f));

                PhysicsSystem::getInstance().createRigidBody(
                    chunkPtr,
                    BodyType::Dynamic,
                    physicsSize,
                    ColliderType::Box,
                    0.7f,
                    0.05f
                );

                glm::vec3 direction = centerWorld - buildingCenter;
                direction.y = 0.0f;

                if (glm::length(direction) < 0.001f) {
                    direction = glm::vec3(jitterDist(rng), 0.0f, jitterDist(rng));
                }

                if (glm::length(direction) > 0.001f) {
                    direction = glm::normalize(direction);
                }

                const glm::vec3 velocity =
                    direction * speedDist(rng) +
                    glm::vec3(
                        jitterDist(rng) * 0.3f,
                        upDist(rng),
                        jitterDist(rng) * 0.3f
                    );

                PhysicsSystem::getInstance().setLinearVelocity(chunkPtr, velocity);

                ++chunkIndex;
            }
        }
    }
}