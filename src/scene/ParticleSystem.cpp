#include "scene/ParticleSystem.h"
#include "rendering/Shader.h"

#include <glad/glad.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>

namespace {

const char* particleVertexShader = R"glsl(#version 450 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;

uniform mat4 uView;
uniform mat4 uProjection;

out vec4 vColor;

void main() {
    vec4 viewPosition = uView * vec4(aPosition, 1.0);

    gl_Position = uProjection * viewPosition;

    float distance = max(length(viewPosition.xyz), 0.1);

    // Tune 600.0 if particles look too small/large.
    float pointSize = aSize * 600.0 / distance;

    gl_PointSize = clamp(pointSize, 1.0, 256.0);

    vColor = aColor;
}
)glsl";

const char* particleFragmentShader = R"glsl(#version 450 core
in vec4 vColor;

out vec4 FragColor;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float distanceSquared = dot(uv, uv);

    if (distanceSquared > 1.0) {
        discard;
    }

    float alpha = smoothstep(1.0, 0.2, distanceSquared);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)glsl";

Shader* getParticleShader() {
    static std::unique_ptr<Shader> shader;

    if (!shader) {
        shader = std::make_unique<Shader>(
            particleVertexShader,
            particleFragmentShader
        );
    }

    return shader.get();
}

}

ParticleSystem::ParticleSystem(const std::string& name, int maxParticles)
    : Node(name) {
    m_maxParticles = std::max(1, maxParticles);
    m_particles.reserve(static_cast<size_t>(m_maxParticles));
    m_vertexData.reserve(static_cast<size_t>(m_maxParticles) * 8);
}

ParticleSystem::~ParticleSystem() {
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
}

std::mt19937& ParticleSystem::randomGenerator() {
    static std::mt19937 generator{std::random_device{}()};
    return generator;
}

float ParticleSystem::randomRange(float minValue, float maxValue) {
    if (minValue >= maxValue) {
        return minValue;
    }

    std::uniform_real_distribution<float> distribution(minValue, maxValue);
    return distribution(randomGenerator());
}

glm::vec3 ParticleSystem::randomInSphere(float radius) {
    glm::vec3 direction(
        randomRange(-1.0f, 1.0f),
        randomRange(-1.0f, 1.0f),
        randomRange(-1.0f, 1.0f)
    );

    if (glm::length(direction) < 1e-4f) {
        direction = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    direction = glm::normalize(direction);

    const float r = radius * std::pow(randomRange(0.0f, 1.0f), 1.0f / 3.0f);

    return direction * r;
}

glm::vec3 ParticleSystem::randomConeDirection(
    const glm::vec3& baseDirection,
    float spreadRadians
) {
    glm::vec3 direction = baseDirection;

    const float length = glm::length(direction);

    if (length < 1e-5f) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    direction /= length;

    if (spreadRadians <= 0.0f) {
        return direction;
    }

    const float angle = randomRange(0.0f, spreadRadians);
    const float phi = randomRange(0.0f, glm::two_pi<float>());

    const glm::vec3 helper =
        std::abs(direction.y) < 0.99f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);

    const glm::vec3 right = glm::normalize(glm::cross(helper, direction));
    const glm::vec3 up = glm::cross(direction, right);

    const glm::vec3 result =
        direction * std::cos(angle) +
        (right * std::cos(phi) + up * std::sin(phi)) * std::sin(angle);

    return glm::normalize(result);
}

void ParticleSystem::clear() {
    m_particles.clear();
    m_emitAccumulator = 0.0f;
}

void ParticleSystem::initializeParticle(
    Particle& particle,
    const glm::mat4& worldTransform
) {
    particle.age = 0.0f;
    particle.lifetime = randomRange(m_minLifetime, m_maxLifetime);

    particle.startSize = m_startSize;
    particle.endSize = m_endSize;

    particle.seed = static_cast<unsigned int>(randomGenerator()());

    const glm::vec3 localOffset = randomInSphere(m_spawnRadius);

    particle.position = glm::vec3(
        worldTransform * glm::vec4(localOffset, 1.0f)
    );

    glm::vec3 worldDirection = glm::vec3(
        worldTransform * glm::vec4(m_baseDirection, 0.0f)
    );

    if (glm::length(worldDirection) < 1e-5f) {
        worldDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        worldDirection = glm::normalize(worldDirection);
    }

    particle.velocity =
        randomConeDirection(worldDirection, m_spread) *
        randomRange(m_minSpeed, m_maxSpeed);

    particle.startColor = glm::vec4(1.0f);
    particle.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
}

void ParticleSystem::updateParticle(Particle& particle, float deltaTime) {
    particle.velocity += m_gravity * deltaTime;
    particle.position += particle.velocity * deltaTime;
}

void ParticleSystem::emitParticles(
    int count,
    const glm::mat4& worldTransform
) {
    for (int i = 0; i < count; ++i) {
        if (static_cast<int>(m_particles.size()) >= m_maxParticles) {
            break;
        }

        Particle particle;
        initializeParticle(particle, worldTransform);
        m_particles.push_back(particle);
    }
}

void ParticleSystem::update(float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }

    m_particles.erase(
        std::remove_if(
            m_particles.begin(),
            m_particles.end(),
            [](const Particle& particle) {
                return particle.age >= particle.lifetime;
            }
        ),
        m_particles.end()
    );

    if (m_playing && m_emissionRate > 0.0f) {
        m_emitAccumulator += deltaTime * m_emissionRate;

        int spawnCount = static_cast<int>(m_emitAccumulator);

        if (spawnCount > 0) {
            m_emitAccumulator -= static_cast<float>(spawnCount);

            const glm::mat4 worldTransform = getWorldTransform();
            emitParticles(spawnCount, worldTransform);
        }
    }

    for (Particle& particle : m_particles) {
        particle.age += deltaTime;

        if (particle.age < particle.lifetime) {
            updateParticle(particle, deltaTime);
        }
    }

    m_particles.erase(
        std::remove_if(
            m_particles.begin(),
            m_particles.end(),
            [](const Particle& particle) {
                return particle.age >= particle.lifetime;
            }
        ),
        m_particles.end()
    );
}

void ParticleSystem::initGraphics() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        4,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        1,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(7 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    m_graphicsInitialized = true;
}

void ParticleSystem::render(
    const glm::mat4& view,
    const glm::mat4& projection
) {
    if (m_particles.empty()) {
        return;
    }

    if (!m_graphicsInitialized) {
        initGraphics();
    }

    m_vertexData.clear();
    m_vertexData.reserve(m_particles.size() * 8);

    for (const Particle& particle : m_particles) {
        const float t = glm::clamp(
            particle.age / particle.lifetime,
            0.0f,
            1.0f
        );

        const glm::vec4 color = glm::mix(
            particle.startColor,
            particle.endColor,
            t
        );

        const float size = glm::mix(
            particle.startSize,
            particle.endSize,
            t
        );

        m_vertexData.push_back(particle.position.x);
        m_vertexData.push_back(particle.position.y);
        m_vertexData.push_back(particle.position.z);

        m_vertexData.push_back(color.r);
        m_vertexData.push_back(color.g);
        m_vertexData.push_back(color.b);
        m_vertexData.push_back(color.a);

        m_vertexData.push_back(size);
    }

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    if (getBlendMode() == BlendMode::Additive) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    glEnable(GL_PROGRAM_POINT_SIZE);

    Shader* shader = getParticleShader();

    shader->use();
    shader->setMat4("uView", view);
    shader->setMat4("uProjection", projection);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        m_vertexData.size() * sizeof(float),
        m_vertexData.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(
        GL_POINTS,
        0,
        static_cast<GLsizei>(m_particles.size())
    );

    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_ONE, GL_ZERO);
}

void updateParticleSystems(Node* root, float deltaTime) {
    if (root == nullptr) {
        return;
    }

    if (auto* particleSystem = dynamic_cast<ParticleSystem*>(root)) {
        particleSystem->update(deltaTime);
    }

    for (const auto& child : root->getChildren()) {
        updateParticleSystems(child.get(), deltaTime);
    }
}

void renderParticleSystems(
    Node* root,
    const glm::mat4& view,
    const glm::mat4& projection
) {
    if (root == nullptr) {
        return;
    }

    if (auto* particleSystem = dynamic_cast<ParticleSystem*>(root)) {
        particleSystem->render(view, projection);
    }

    for (const auto& child : root->getChildren()) {
        renderParticleSystems(child.get(), view, projection);
    }
}