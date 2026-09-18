#pragma once
#include "scene/Node.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#ifdef CHISEL_ENABLE_JOLT
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystem.h>
#endif

enum class BodyType { Static, Dynamic, Kinematic };

struct PhysicsDebugLine {
    glm::vec3 from;
    glm::vec3 to;
};

class PhysicsBody {
public:
    PhysicsBody(Node* node, BodyType type, const glm::vec3& size);
    ~PhysicsBody();

    void syncFromPhysics();
    void appendDebugLines(std::vector<PhysicsDebugLine>& lines) const;

private:
    friend class PhysicsBody;
    friend class PhysicsSystem;
    Node* m_node;
    BodyType m_type;
    glm::vec3 m_size;
#ifdef CHISEL_ENABLE_JOLT
    JPH::BodyID m_bodyID;
#endif
};

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance();

    void init();
    void shutdown();
    PhysicsBody* createRigidBody(Node* node, BodyType type,
                                 const glm::vec3& size,
                                 float friction = 0.5f,
                                 float restitution = 0.1f);
    void update(float renderDeltaTime);
    void setDebugDrawEnabled(bool enabled) { m_debugDrawEnabled = enabled; }
    bool isDebugDrawEnabled() const { return m_debugDrawEnabled; }
    std::vector<PhysicsDebugLine> getDebugLines() const;

private:
    friend class PhysicsBody;
    PhysicsSystem() = default;
    ~PhysicsSystem();
    PhysicsSystem(const PhysicsSystem&) = delete;
    PhysicsSystem& operator=(const PhysicsSystem&) = delete;

    std::vector<std::unique_ptr<PhysicsBody>> m_bodies;
    double m_accumulator = 0.0;
    bool m_initialized = false;
    bool m_debugDrawEnabled = false;
#ifdef CHISEL_ENABLE_JOLT
    JPH::PhysicsSystem m_physicsSystem;
    JPH::TempAllocator* m_tempAllocator = nullptr;
    JPH::JobSystem* m_jobSystem = nullptr;
#endif
};
