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
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
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

void PhysicsBody::syncToPhysics() {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid())
        return;
    auto& bodyInterface = PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface();
    if (m_type == BodyType::Dynamic) {
        bodyInterface.SetLinearVelocity(m_bodyID, JPH::Vec3::sZero());
        bodyInterface.SetAngularVelocity(m_bodyID, JPH::Vec3::sZero());
    }

    bodyInterface.SetPositionAndRotation(
        m_bodyID,
        JPH::RVec3(m_node->getPosition().x, m_node->getPosition().y, m_node->getPosition().z),
        JPH::Quat(m_node->getRotation().x, m_node->getRotation().y,
                  m_node->getRotation().z, m_node->getRotation().w),
        JPH::EActivation::DontActivate);
#endif
}

void PhysicsBody::beginEditorManipulation() {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid() || m_type != BodyType::Dynamic)
        return;
    auto& bodyInterface = PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface();
    bodyInterface.SetLinearVelocity(m_bodyID, JPH::Vec3::sZero());
    bodyInterface.SetAngularVelocity(m_bodyID, JPH::Vec3::sZero());
    bodyInterface.SetMotionType(m_bodyID, JPH::EMotionType::Kinematic,
                                JPH::EActivation::DontActivate);
#endif
}

void PhysicsBody::endEditorManipulation() {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid() || m_type != BodyType::Dynamic)
        return;
    auto& bodyInterface = PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface();
    bodyInterface.SetMotionType(m_bodyID, JPH::EMotionType::Dynamic,
                                JPH::EActivation::Activate);
    bodyInterface.SetLinearVelocity(m_bodyID, JPH::Vec3::sZero());
    bodyInterface.SetAngularVelocity(m_bodyID, JPH::Vec3::sZero());
#endif
}

void PhysicsBody::appendDebugLines(std::vector<PhysicsDebugLine>& lines) const {
#ifdef CHISEL_ENABLE_JOLT
    if (m_bodyID.IsInvalid() || m_shape == nullptr)
        return;
    JPH::RVec3 position;
    JPH::Quat rotation;
    PhysicsSystem::getInstance().m_physicsSystem.GetBodyInterface().GetPositionAndRotation(
        m_bodyID, position, rotation);
    const glm::quat q(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
    const glm::vec3 center(static_cast<float>(position.GetX()),
                           static_cast<float>(position.GetY()),
                           static_cast<float>(position.GetZ()));
    if (const auto* hull = dynamic_cast<const JPH::ConvexHullShape*>(m_shape.GetPtr())) {
        for (uint32_t faceIndex = 0; faceIndex < hull->GetNumFaces(); ++faceIndex) {
            const uint32_t count = hull->GetNumVerticesInFace(faceIndex);
            std::vector<uint32_t> face(count);
            hull->GetFaceVertices(faceIndex, count, face.data());
            for (uint32_t i = 0; i < count; ++i) {
                const JPH::Vec3 a = hull->GetPoint(face[i]);
                const JPH::Vec3 b = hull->GetPoint(face[(i + 1) % count]);
                const glm::vec3 from = center + q * glm::vec3(a.GetX(), a.GetY(), a.GetZ());
                const glm::vec3 to = center + q * glm::vec3(b.GetX(), b.GetY(), b.GetZ());
                lines.push_back({from, to});
            }
        }
    }
#else
    (void)lines;
#endif
}

PhysicsSystem& PhysicsSystem::getInstance() {
    static PhysicsSystem instance;
    return instance;
}

void PhysicsSystem::beginEditorManipulation(Node* node) {
    for (auto& body : m_bodies)
        if (body->m_node == node)
            body->beginEditorManipulation();
}

void PhysicsSystem::endEditorManipulation(Node* node) {
    for (auto& body : m_bodies)
        if (body->m_node == node)
            body->endEditorManipulation();
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
    m_physicsSystem.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
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
    std::vector<JPH::Vec3> hullPoints;
    if (const auto* mesh = dynamic_cast<const MeshNode*>(node)) {
        constexpr float epsilon = 1.0e-4f;
        for (const glm::vec3& point : mesh->getCollisionVertices()) {
            bool duplicate = false;
            for (const JPH::Vec3& existing : hullPoints) {
                if ((point.x - existing.GetX()) * (point.x - existing.GetX()) +
                    (point.y - existing.GetY()) * (point.y - existing.GetY()) +
                    (point.z - existing.GetZ()) * (point.z - existing.GetZ()) <
                    epsilon * epsilon) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
                hullPoints.emplace_back(point.x, point.y, point.z);
        }
    }
    if (hullPoints.size() < 4) {
        const glm::vec3 safeSize = glm::max(size, glm::vec3(0.01f));
        JPH::BoxShapeSettings shapeSettings(
            JPH::Vec3(safeSize.x * 0.5f, safeSize.y * 0.5f, safeSize.z * 0.5f),
            0.0f);
        const JPH::ShapeSettings::ShapeResult shape = shapeSettings.Create();
        if (shape.HasError()) {
            std::cerr << "Jolt fallback box shape creation failed: "
                      << shape.GetError() << std::endl;
            return nullptr;
        }
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
        if (bodyID.IsInvalid()) {
            std::cerr << "Jolt rigid body creation failed" << std::endl;
            return nullptr;
        }
        result->m_bodyID = bodyID;
        result->m_shape = shape.Get();
        m_bodies.push_back(std::move(body));
        return result;
    }
    JPH::ConvexHullShapeSettings shapeSettings(hullPoints.data(),
                                                static_cast<int>(hullPoints.size()));
    const JPH::ShapeSettings::ShapeResult shape = shapeSettings.Create();
    if (shape.HasError()) {
        std::cerr << "Jolt shape creation failed: " << shape.GetError() << std::endl;
        return nullptr;
    }
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
    if (bodyID.IsInvalid()) {
        std::cerr << "Jolt rigid body creation failed" << std::endl;
        return nullptr;
    }
    result->m_bodyID = bodyID;
    result->m_shape = shape.Get();
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
    for (auto& body : m_bodies) {
        if (body->m_node->isEditorManipulated())
            body->syncToPhysics();
        else
            body->syncFromPhysics();
    }
}

std::vector<PhysicsDebugLine> PhysicsSystem::getDebugLines() const {
    std::vector<PhysicsDebugLine> lines;
    if (!m_debugDrawEnabled)
        return lines;
    for (const auto& body : m_bodies)
        body->appendDebugLines(lines);
    return lines;
}
