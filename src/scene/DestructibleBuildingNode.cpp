#include "scene/DestructibleBuildingNode.h"
#include "scene/Scene.h"
#include "platform/PhysicsSystem.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace {

struct MeshData {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
};

float tetrahedronVolume(
    const glm::vec3& a,
    const glm::vec3& b,
    const glm::vec3& c,
    const glm::vec3& d
) {
    return std::abs(glm::dot(glm::cross(b - a, c - a), d - a)) / 6.0f;
}

MeshData buildBoxMesh(const glm::vec3& min, const glm::vec3& max) {
    MeshData out;

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

    const MeshData mesh = buildBoxMesh(min, max);

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

MeshData buildTetrahedronMesh(const std::array<glm::vec3, 4>& points) {
    MeshData out;
    const int faces[4][3] = {
        {1, 2, 3},
        {0, 3, 2},
        {0, 1, 3},
        {0, 2, 1}
    };

    for (const auto& face : faces) {
        int i0 = face[0];
        int i1 = face[1];
        int i2 = face[2];

        glm::vec3 a = points[i0];
        glm::vec3 b = points[i1];
        glm::vec3 c = points[i2];

        glm::vec3 normal = glm::cross(b - a, c - a);
        const glm::vec3 faceCenter = (a + b + c) / 3.0f;

        if (glm::dot(normal, faceCenter) < 0.0f) {
            std::swap(i1, i2);
            std::swap(b, c);
            normal = -normal;
        }

        if (glm::length(normal) > 1e-6f) {
            normal = glm::normalize(normal);
        } else {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        const unsigned int base =
            static_cast<unsigned int>(out.vertices.size() / 8);

        const glm::vec3 triangle[3] = {a, b, c};

        for (const glm::vec3& position : triangle) {
            out.vertices.push_back(position.x);
            out.vertices.push_back(position.y);
            out.vertices.push_back(position.z);
            out.vertices.push_back(normal.x);
            out.vertices.push_back(normal.y);
            out.vertices.push_back(normal.z);
            out.vertices.push_back(position.x);
            out.vertices.push_back(position.y);
        }

        out.indices.insert(
            out.indices.end(),
            {
                base,
                base + 1,
                base + 2
            }
        );
    }

    return out;
}

void setupShardMesh(
    MeshNode* node,
    const std::array<glm::vec3, 4>& localPoints
) {
    if (node == nullptr) {
        return;
    }

    const MeshData mesh = buildTetrahedronMesh(localPoints);

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

    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());

    for (const glm::vec3& point : localPoints) {
        boundsMin = glm::min(boundsMin, point);
        boundsMax = glm::max(boundsMax, point);
    }

    node->setBounds(boundsMin, boundsMax);

    std::vector<glm::vec3> collisionVertices(
        localPoints.begin(),
        localPoints.end()
    );

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

    const glm::vec3 buildingMin(
        -m_size.x * 0.5f,
        0.0f,
        -m_size.z * 0.5f
    );

    const glm::vec3 buildingMax(
        m_size.x * 0.5f,
        m_size.y,
        m_size.z * 0.5f
    );

    const int xSegments = 2;
    const int ySegments = 4;
    const int zSegments = 1;

    const float dx = m_size.x / static_cast<float>(xSegments);
    const float dy = m_size.y / static_cast<float>(ySegments);
    const float dz = m_size.z / static_cast<float>(zSegments);

    const glm::vec3 cellSize(dx, dy, dz);

    const int nx = xSegments + 1;
    const int ny = ySegments + 1;
    const int nz = zSegments + 1;

    static std::mt19937 rng{std::random_device{}()};

    std::uniform_real_distribution<float> jitterDist(-0.5f, 0.5f);
    std::uniform_real_distribution<float> speedDist(1.5f, 4.0f);
    std::uniform_real_distribution<float> upDist(1.0f, 3.0f);
    std::uniform_real_distribution<float> velocityJitterDist(-0.25f, 0.25f);
    std::uniform_real_distribution<float> shadeDist(0.80f, 1.05f);

    auto gridIndex = [&](int x, int y, int z) {
        return (z * ny + y) * nx + x;
    };

    std::vector<glm::vec3> grid(static_cast<size_t>(nx * ny * nz));

    for (int z = 0; z < nz; ++z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                glm::vec3 p = buildingMin + glm::vec3(
                    static_cast<float>(x) * dx,
                    static_cast<float>(y) * dy,
                    static_cast<float>(z) * dz
                );

                const bool isCorner =
                    (x == 0 || x == xSegments) &&
                    (y == 0 || y == ySegments) &&
                    (z == 0 || z == zSegments);

                if (!isCorner) {
                    const glm::vec3 jitter(
                        jitterDist(rng),
                        jitterDist(rng),
                        jitterDist(rng)
                    );

                    p += jitter * cellSize * 0.18f;
                    p = glm::max(glm::min(p, buildingMax), buildingMin);
                }

                grid[static_cast<size_t>(gridIndex(x, y, z))] = p;
            }
        }
    }

    const int tetIndices[6][4] = {
        {0, 1, 2, 6},
        {0, 2, 3, 6},
        {0, 3, 7, 6},
        {0, 7, 4, 6},
        {0, 4, 5, 6},
        {0, 5, 1, 6}
    };

    int shardIndex = 0;

    for (int z = 0; z < zSegments; ++z) {
        for (int y = 0; y < ySegments; ++y) {
            for (int x = 0; x < xSegments; ++x) {
                const int c[8] = {
                    gridIndex(x,     y,     z),
                    gridIndex(x + 1, y,     z),
                    gridIndex(x + 1, y + 1, z),
                    gridIndex(x,     y + 1, z),
                    gridIndex(x,     y,     z + 1),
                    gridIndex(x + 1, y,     z + 1),
                    gridIndex(x + 1, y + 1, z + 1),
                    gridIndex(x,     y + 1, z + 1)
                };

                for (const auto& tet : tetIndices) {
                    const std::array<glm::vec3, 4> buildingSpacePoints = {
                        grid[static_cast<size_t>(c[tet[0]])],
                        grid[static_cast<size_t>(c[tet[1]])],
                        grid[static_cast<size_t>(c[tet[2]])],
                        grid[static_cast<size_t>(c[tet[3]])]
                    };

                    const float volume = tetrahedronVolume(
                        buildingSpacePoints[0],
                        buildingSpacePoints[1],
                        buildingSpacePoints[2],
                        buildingSpacePoints[3]
                    );

                    if (volume < 1e-6f) {
                        continue;
                    }

                    const glm::vec3 centroid =
                        (
                            buildingSpacePoints[0] +
                            buildingSpacePoints[1] +
                            buildingSpacePoints[2] +
                            buildingSpacePoints[3]
                        ) * 0.25f;

                    std::array<glm::vec3, 4> localPoints{};

                    for (int i = 0; i < 4; ++i) {
                        localPoints[static_cast<size_t>(i)] =
                            (buildingSpacePoints[static_cast<size_t>(i)] - centroid) * 0.94f;
                    }

                    const glm::vec3 worldCentroid = glm::vec3(
                        cachedWorldTransform * glm::vec4(centroid, 1.0f)
                    );

                    auto shard = std::make_unique<MeshNode>(
                        getName() + "_shard_" + std::to_string(shardIndex)
                    );

                    setupShardMesh(shard.get(), localPoints);

                    shard->setPosition(worldCentroid);
                    shard->setRotation(buildingRotation);

                    Material shardMaterial = m_material;

                    glm::vec3 color = glm::vec3(shardMaterial.baseColorFactor);
                    color *= shadeDist(rng);
                    color = glm::max(glm::min(color, glm::vec3(1.0f)), glm::vec3(0.0f));

                    shardMaterial.baseColorFactor = glm::vec4(color, 1.0f);
                    shard->setMaterial(shardMaterial);

                    MeshNode* shardPtr = shard.get();

                    (void)m_scene->adopt(std::move(shard));

                    const glm::vec3 physicsSize =
                        glm::max(shardPtr->getBoundsSize(), glm::vec3(0.01f));

                    PhysicsSystem::getInstance().createRigidBody(
                        shardPtr,
                        BodyType::Dynamic,
                        physicsSize,
                        ColliderType::Convex,
                        0.6f,
                        0.05f
                    );

                    glm::vec3 direction = worldCentroid - buildingCenter;
                    direction.y *= 0.35f;

                    if (glm::length(direction) < 0.001f) {
                        direction = glm::vec3(
                            velocityJitterDist(rng),
                            0.0f,
                            velocityJitterDist(rng)
                        );
                    }

                    if (glm::length(direction) > 0.001f) {
                        direction = glm::normalize(direction);
                    }

                    const glm::vec3 velocity =
                        direction * speedDist(rng) +
                        glm::vec3(
                            velocityJitterDist(rng),
                            upDist(rng),
                            velocityJitterDist(rng)
                        );

                    PhysicsSystem::getInstance().setLinearVelocity(shardPtr, velocity);

                    ++shardIndex;
                }
            }
        }
    }
}