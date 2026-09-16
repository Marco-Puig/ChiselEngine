#include "Platform/PhysicsSystem.h"
#include "Scene/Node.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <iostream>

// Helper to convert GLM to Jolt
JPH::Vec3 glmToJolt(glm::vec3 v) { return JPH::Vec3(v.x, v.y, v.z); }
glm::vec3 joltToGlm(JPH::Vec3 v) { return glm::vec3(v.x, v.y, v.z); }
JPH::Quat glmToJoltQuat(glm::quat q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
glm::quat joltToGlmQuat(JPH::Quat q) { return glm::quat(q.w, q.x, q.y, q.z); }

PhysicsBody::PhysicsBody(Node* targetNode, JPH::BodyID bodyID) 
    : m_targetNode(targetNode), m_bodyID(bodyID) {}

PhysicsBody::~PhysicsBody() {
    // bodyInterface.RemoveBody(m_bodyID);
}

void PhysicsBody::setMass(float mass) {
    // Jolt implementation uses mass properties
}

void PhysicsBody::applyForce(glm::vec3 force) {
    JPH::PhysicsSystem::GetSingleton().GetBodyInterface().AddForce(m_bodyID, glmToJolt(force));
}

void PhysicsBody::setVelocity(glm::vec3 vel) {
    JPH::PhysicsSystem::GetSingleton().GetBodyInterface().SetLinearVelocity(m_bodyID, glmToJolt(vel));
}

void PhysicsBody::syncToNode() {
    JPH::RVec3 position;
    JPH::Quat rotation;
    JPH::PhysicsSystem::GetSingleton().GetBodyInterface().GetPositionAndRotation(m_bodyID, position, rotation);
    
    m_targetNode->setPosition(joltToGlm(position));
    m_targetNode->setRotation(joltToGlmQuat(rotation));
}

bool PhysicsSystem::init() {
    // 1. Initialize Jolt Core
    JPH::RegisterDefaultPhysics();
    
    m_threadPool = new JPH::JobSystemThreadPool(std::thread::hardware_concurrency(), std::thread::hardware_concurrency());
    m_tempAllocator = new JPH::TempAllocator();
    
    // 2. Create Physics System
    m_physicsSystem = new JPH::PhysicsSystem();
    
    return true;
}

void PhysicsSystem::update(float deltaTime) {
    if (!m_physicsSystem) return;

    // Step simulation
    m_physicsSystem->Update(deltaTime, 1, 1, m_tempAllocator, m_threadPool);

    // Sync bodies back to nodes
    for (auto body : m_bodies) {
        body->syncToNode();
    }
}

void PhysicsSystem::shutdown() {
    delete m_physicsSystem;
    delete m_threadPool;
    delete m_tempAllocator;
}

PhysicsBody* PhysicsSystem::createRigidBody(Node* node, BodyType type, float mass, glm::vec3 size) {
    // 1. Create Shape
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(size.x / 2, size.y / 2, size.z / 2));
    JPH::ShapeSettings shapeSettings_base = shapeSettings;
    JPH::Shape* shape = shapeSettings_base.Create();

    // 2. Create Body Settings
    JPH::BodyCreationSettings settings(node->getPosition(), glmToJoltQuat(node->getRotation()), shape, 
        (type == BodyType::Dynamic) ? JPH::EMotionType::Dynamic : 
        (type == BodyType::Kinematic) ? JPH::EMotionType::Kinematic : JPH::EMotionType::Static);

    if (type == BodyType::Dynamic) {
        settings.SetMassProperties(mass);
    }

    // 3. Create the body in the world
    JPH::BodyID id = m_physicsSystem->CreateBody(settings);
    
    PhysicsBody* body = new PhysicsBody(node, id);
    m_bodies.push_back(body);
    return body;
}
