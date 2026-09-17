#include "Game.h"
#include "rendering/RenderSystem.h"
#include "rendering/GLBLoader.h"
#include "rendering/Light.h"
#include "scene/MeshNode.h"
#include "scene/Animator.h"
#include <iostream>

Game::Game() : sceneRoot(std::make_unique<Node>("Root")) {}

void Game::start() {
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    RenderSystem::getInstance().addLight(sun);

    MeshNode* cubeMesh = GLBLoader::loadGLB("resources/cube.glb");
    cube = cubeMesh;
    sceneRoot->addChild(std::unique_ptr<Node>(cubeMesh));
    animator = std::make_unique<Animator>(cube);
    animator->procedural(cube, Axis::Y, Direction::Positive,
                         TransformType::Rotation, glm::radians(45.0f));
}

void Game::update(float deltaTime) {
    if (animator != nullptr)
        animator->update(deltaTime);
}
