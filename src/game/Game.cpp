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

    /* Temporarily disabled until Assimp build issues are resolved
    MeshNode* playerSword = GLBLoader::loadGLB("Resources/sword.glb");
    if (playerSword) {
        sceneRoot->addChild(std::unique_ptr<Node>(playerSword));
        swordAnimator = new Animator(playerSword);
    }
    */
}

void Game::update(float deltaTime) {
    // Simplified input check: in real engine use Input::isButtonPressed
    bool buttonA = false; 
    if (buttonA && swordAnimator) {
        swordAnimator->playAnimation("Attack_Swing");
    }
}
