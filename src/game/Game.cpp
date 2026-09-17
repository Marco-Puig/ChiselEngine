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

    MeshNode* cube = GLBLoader::loadGLB("resources/cube.glb");
    sceneRoot->addChild(std::unique_ptr<Node>(cube));
}

void Game::update(float deltaTime) {
    (void)deltaTime;
}
