#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

// Jolt Headers
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>

enum class BodyType {
    Static,
    Kinematic,
    Dynamic
};

class PhysicsBody {
public:
    PhysicsBody(class Node* targetNode, JPH::BodyID bodyID);
    ~PhysicsBody();

    void setMass(float mass);
    void applyForce(glm::vec3 force);
    void setVelocity(glm::vec3 vel);
    void syncToNode();

private:
    class Node* m_targetNode;
    JPH::BodyID m_bodyID;
};

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance() {
        static PhysicsSystem instance;
        return instance;
    }

    bool init();
    void update(float deltaTime);
    void shutdown();

    PhysicsBody* createRigidBody(class Node* node, BodyType type, float mass = 1.0f, glm::vec3 size = glm::vec3(1.0f));

private:
    PhysicsSystem() = default;

    JPH::PhysicsSystem* m_physicsSystem = nullptr;
    JPH::JobSystemThreadPool* m_threadPool = nullptr;
    JPH::TempAllocator* m_tempAllocator = nullptr;
    
    std::vector<PhysicsBody*> m_bodies;
};
