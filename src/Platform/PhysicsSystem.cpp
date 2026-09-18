#include "PhysicsSystem.h"
#include "scene/MeshNode.h"
#include <array>
#include <algorithm>

#ifdef CHISEL_ENABLE_JOLT
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

namespace {
constexpr JPH::ObjectLayer cNonMoving = 0;
constexpr JPH::ObjectLayer cMoving = 1;
constexpr JPH::BroadPhaseLayer cBroadPhaseNonMoving(0);
constexpr JPH::BroadPhaseLayer cBroadPhaseMoving(1);

class BroadPhaseLayers final : public JPH::BroadPhaseLayerInterface {
public:
    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return layer == cMoving ? cBroadPhaseMoving : cBroadPhaseNonMoving;
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        return layer == cBroadPhaseMoving ? "Moving" : "NonMoving";
    }
#endif
};

class ObjectVsBroadPhase final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhase) const override {
        return layer == cMoving || broadPhase == cBroadPhaseMoving;
    }
};

class ObjectLayerPair final : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override {
        return first == cMoving || second == cMoving;
    }
};

BroadPhaseLayers gBroadPhaseLayers;
ObjectVsBroadPhase gObjectVsBroadPhase;
ObjectLayerPair gObjectLayerPair;
}
#endif

PhysicsBody::PhysicsBody(Node* node, BodyType type, const glm::vec3& size)
    : m_node(node), m_type(type), m_size(size)
#ifdef CHISEL_ENABLE_JOLT
    , m_bodyID(JPH::BodyID())
#endif
{
}

PhysicsBody::~PhysicsBody() = default;

void PhysicsBody::syncFromPhysics() {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid())
        return;
    JPH::RVec3 position;
    JPH::Quat rotation;
    PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface().GetPositionAndRotation(
        m_bodyID, position, rotation);
    m_node->setPosition(glm::vec3(static_cast<float>(position.GetX()),
                                  static_cast<float>(position.GetY()),
                                  static_cast<float>(position.GetZ())));
    m_node->setRotation(glm::quat(rotation.GetW(), rotation.GetX(),
                                  rotation.GetY(), rotation.GetZ()));
#endif
}

void PhysicsBody::appendDebugLines(std::vector<PhysicsDebugLine>& lines) const {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid())
        return;
    JPH::RVec3 position;
    JPH::Quat rotation;
    PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface().GetPositionAndRotation(
        m_bodyID, position, rotation);
    const glm::quat q(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    const glm::vec3 center(static_cast<float>(position.GetX()),
                           static_cast<float>(position.GetY()),
                           static_cast<float>(position.GetZ()));
    const glm::vec3 half = glm::max(m_size, glm::vec3(0.01f)) * 0.5f;
    const std::array<glm::vec3, 8> corners = {
        glm::vec3(-half.x, -half.y, -half.z), glm::vec3(half.x, -half.y, -half.z),
        glm::vec3(half.x, half.y, -half.z), glm::vec3(-half.x, half.y, -half.z),
        glm::vec3(-half.x, -half.y, half.z), glm::vec3(half.x, -half.y, half.z),
        glm::vec3(half.x, half.y, half.z), glm::vec3(-half.x, half.y, half.z)};
    constexpr int edges[][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
        {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (const auto& edge : edges)
        lines.push_back({center + q * corners[edge[0]], center + q * corners[edge[1]]});
#else
    (void)lines;
#endif
}

PhysicsSystem& PhysicsSystem::getInstance() {
    static PhysicsSystem instance;
    return instance;
}

PhysicsSystem::~PhysicsSystem() {
    shutdown();
}

void PhysicsSystem::init() {
    if (m_initialized)
        return;
#ifdef CHISEL_ENABLE_JOLT
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    m_tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
    m_jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers);
    m_physicsSystem.Init(10240, 0, 10240, 10240, gBroadPhaseLayers,
                         gObjectVsBroadPhase, gObjectLayerPair);
#endif
    m_initialized = true;
}

void PhysicsSystem::shutdown() {
    if (!m_initialized)
        return;
    m_bodies.clear();
#ifdef CHISEL_ENABLE_JOLT
    delete m_jobSystem;
    delete m_tempAllocator;
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
#endif
    m_accumulator = 0.0;
    m_initialized = false;
}

PhysicsBody* PhysicsSystem::createRigidBody(Node* node, BodyType type,
                                             const glm::vec3& size,
                                             float friction, float restitution) {
    if (!m_initialized)
        init();
    auto body = std::make_unique<PhysicsBody>(node, type, size);
    PhysicsBody* result = body.get();
#ifdef CHISEL_ENABLE_JOLT
    const glm::vec3 safeSize = glm::max(size, glm::vec3(0.01f));
    JPH::BoxShapeSettings shapeSettings(
        JPH::Vec3(safeSize.x * 0.5f, safeSize.y * 0.5f, safeSize.z * 0.5f));
    const JPH::ShapeSettings::ShapeResult shape = shapeSettings.Create();
    if (shape.HasError())
        return nullptr;
    const JPH::EMotionType motion = type == BodyType::Static ? JPH::EMotionType::Static :
        type == BodyType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
    const JPH::ObjectLayer layer = motion == JPH::EMotionType::Static ? cNonMoving : cMoving;
    JPH::BodyCreationSettings settings(
        shape.Get(), JPH::RVec3(node->getPosition().x, node->getPosition().y, node->getPosition().z),
        JPH::Quat(node->getRotation().x, node->getRotation().y,
                  node->getRotation().z, node->getRotation().w),
        motion, layer);
    settings.mFriction = friction;
    settings.mRestitution = restitution;
    const JPH::BodyID bodyID = m_physicsSystem.GetBodyInterface().CreateAndAddBody(
        settings, motion == JPH::EMotionType::Static ? JPH::EActivation::DontActivate :
        JPH::EActivation::Activate);
    if (bodyID.IsInvalid())
        return nullptr;
    result->m_bodyID = bodyID;
#else
    (void)friction;
    (void)restitution;
#endif
    m_bodies.push_back(std::move(body));
    return result;
}

void PhysicsSystem::update(float renderDeltaTime) {
    if (!m_initialized)
        return;
    m_accumulator += std::min(static_cast<double>(renderDeltaTime), 0.25);
    constexpr double fixedStep = 1.0 / 60.0;
    while (m_accumulator >= fixedStep) {
#ifdef CHISEL_ENABLE_JOLT
        m_physicsSystem.Update(static_cast<float>(fixedStep), 1,
                               m_tempAllocator, m_jobSystem);
#endif
        m_accumulator -= fixedStep;
    }
    for (auto& body : m_bodies)
        body->syncFromPhysics();
}

std::vector<PhysicsDebugLine> PhysicsSystem::getDebugLines() const {
    std::vector<PhysicsDebugLine> lines;
    if (!m_debugDrawEnabled)
        return lines;
    for (const auto& body : m_bodies)
        body->appendDebugLines(lines);
    return lines;
}
