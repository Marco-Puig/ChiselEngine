#include "Game.h"
#include "rendering/RenderSystem.h"
#include "rendering/GLBLoader.h"
#include "rendering/Light.h"
#include "scene/MeshNode.h"
#include "scene/Animator.h"
#include "platform/PhysicsSystem.h"
#include <stdexcept>

Game::Game()
    : scene(std::make_unique<Scene>()),
      camera(std::make_unique<ArcRotateCamera>("DesktopCamera")) {}

void Game::start() {
    auto sunNode = std::make_unique<DirectionalLight>(
        "Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    DirectionalLight* sun = sunNode.get();
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    sun->setPosition(glm::vec3(0.0f, 4.0f, 0.0f));
    DirectionalLight* ownedSun = scene->adopt(std::move(sunNode));
    RenderSystem::getInstance().addLight(ownedSun);
    RenderSystem::getInstance().setSkyboxPath("resources/skybox.jpg");
    camera->setTarget(glm::vec3(0.0f, 0.5f, 0.0f));
    camera->setLimits(2.0f, 30.0f);

    MeshNode* planeMesh = GLBLoader::loadGLB("resources/plane.glb");
    plane = planeMesh;
    plane->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    scene->adopt(std::unique_ptr<MeshNode>(planeMesh));
    const glm::vec3 floorSize = glm::max(planeMesh->getBoundsSize(),
                                         glm::vec3(0.01f));
    PhysicsBody* floorBody = PhysicsSystem::getInstance().createRigidBody(
        plane, BodyType::Static, floorSize, 0.8f, 0.0f);
    if (floorBody == nullptr)
        throw std::runtime_error("Failed to create the dummy plane collision body");

    MeshNode* frogMesh = GLBLoader::loadGLB("resources/frog.glb");
    frog = frogMesh;
    frog->setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    scene->adopt(std::unique_ptr<MeshNode>(frogMesh));
    PhysicsSystem::getInstance().createRigidBody(
        frog, BodyType::Dynamic, frogMesh->getBoundsSize(), 0.6f, 0.1f);
}

void Game::update(float deltaTime) {
    camera->update(deltaTime);
}
