#pragma once
#include <glm/glm.hpp>
#include "scene/Node.h"

enum class BodyType { Static, Dynamic, Kinematic };

class PhysicsBody {
public:
    PhysicsBody(Node* node, BodyType type) : m_node(node), m_type(type) {}
    
    void syncToNode() {
        // In a real engine, this updates the physics engine with node transform
    }
    
    void syncFromPhysics() {
        // In a real engine, this updates node position from physics engine
    }

private:
    Node* m_node;
    BodyType m_type;
};

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance() {
        static PhysicsSystem instance;
        return instance;
    }

    PhysicsBody* createRigidBody(Node* node, BodyType type) {
        auto body = std::make_unique<PhysicsBody>(node, type);
        PhysicsBody* ptr = body.get();
        m_bodies.push_back(std::move(body));
        return ptr;
    }

    void update(float dt) {
        for (auto& body : m_bodies) {
            body->syncFromPhysics();
        }
    }

private:
    PhysicsSystem() = default;
    std::vector<std::unique_ptr<PhysicsBody>> m_bodies;
};
