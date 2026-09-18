#include "Game.h"
#include "rendering/RenderSystem.h"
#include "rendering/GLBLoader.h"
#include "rendering/Light.h"
#include "scene/MeshNode.h"
#include "scene/Animator.h"
#include "Platform/PhysicsSystem.h"
#include <filesystem>
#include <iostream>

Game::Game()
    : sceneRoot(std::make_unique<Node>("Root")),
      camera(std::make_unique<ArcRotateCamera>("DesktopCamera")) {}

void Game::start() {
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    RenderSystem::getInstance().addLight(sun);
    camera->setTarget(glm::vec3(0.0f, 0.5f, 0.0f));
    camera->setLimits(2.0f, 30.0f);

    const bool hasPlaneAsset = std::filesystem::exists("resources/plane.glb");
    MeshNode* planeMesh = GLBLoader::loadGLB(
        hasPlaneAsset ? "resources/plane.glb" : "resources/cube.glb");
    plane = planeMesh;
    plane->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    if (!hasPlaneAsset)
        plane->setScale(glm::vec3(10.0f, 0.1f, 10.0f));
    sceneRoot->addChild(std::unique_ptr<Node>(planeMesh));
    const glm::vec3 floorSize = planeMesh->getBoundsSize() * plane->getScale();
    PhysicsSystem::getInstance().createRigidBody(
        plane, BodyType::Static, floorSize, 0.8f, 0.0f);

    MeshNode* cubeMesh = GLBLoader::loadGLB("resources/cube.glb");
    cube = cubeMesh;
    cube->setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    sceneRoot->addChild(std::unique_ptr<Node>(cubeMesh));
    PhysicsSystem::getInstance().createRigidBody(
        cube, BodyType::Dynamic, cubeMesh->getBoundsSize(), 0.6f, 0.1f);
}

void Game::update(float deltaTime) {
    camera->update(deltaTime);
}
