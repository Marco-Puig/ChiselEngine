#include "Game.h"
#include "rendering/RenderSystem.h"
#include "rendering/GLBLoader.h"
#include "rendering/Light.h"
#include "scene/MeshNode.h"
#include "scene/Animator.h"
#include "platform/PhysicsSystem.h"
#include <glad/glad.h>
#include <stdexcept>

namespace {
MeshNode* createDummyPlane(float halfExtent) {
    const float vertices[] = {
        -halfExtent, 0.0f, -halfExtent, 0.0f, 1.0f, 0.0f,
         halfExtent, 0.0f, -halfExtent, 0.0f, 1.0f, 0.0f,
         halfExtent, 0.0f,  halfExtent, 0.0f, 1.0f, 0.0f,
        -halfExtent, 0.0f,  halfExtent, 0.0f, 1.0f, 0.0f
    };
    const unsigned int indices[] = {0, 2, 1, 0, 3, 2};

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    auto* mesh = new MeshNode("DummyPlane");
    mesh->setMesh(vao, vbo, ebo, 6, true);
    mesh->setBounds(glm::vec3(-halfExtent, 0.0f, -halfExtent),
                    glm::vec3(halfExtent, 0.0f, halfExtent));
    return mesh;
}
}

Game::Game()
    : sceneRoot(std::make_unique<Node>("Root")),
      camera(std::make_unique<ArcRotateCamera>("DesktopCamera")) {}

void Game::start() {
    DirectionalLight* sun = new DirectionalLight("Sun", glm::vec3(-0.2f, -1.0f, -0.3f));
    sun->setColor(glm::vec3(1.0f, 0.9f, 0.8f));
    RenderSystem::getInstance().addLight(sun);
    RenderSystem::getInstance().setSkyboxPath("resources/skybox.jpg");
    camera->setTarget(glm::vec3(0.0f, 0.5f, 0.0f));
    camera->setLimits(2.0f, 30.0f);

    constexpr float planeHalfExtent = 6.0f;
    MeshNode* planeMesh = createDummyPlane(planeHalfExtent);
    plane = planeMesh;
    plane->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    sceneRoot->addChild(std::unique_ptr<Node>(planeMesh));
    const glm::vec3 floorSize(planeHalfExtent * 2.0f, 0.2f,
                              planeHalfExtent * 2.0f);
    PhysicsBody* floorBody = PhysicsSystem::getInstance().createRigidBody(
        plane, BodyType::Static, floorSize, 0.8f, 0.0f);
    if (floorBody == nullptr)
        throw std::runtime_error("Failed to create the dummy plane collision body");

    MeshNode* cubeMesh = GLBLoader::loadGLB("resources/frog.glb");
    cube = cubeMesh;
    cube->setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
    sceneRoot->addChild(std::unique_ptr<Node>(cubeMesh));
    PhysicsSystem::getInstance().createRigidBody(
        cube, BodyType::Dynamic, cubeMesh->getBoundsSize(), 0.6f, 0.1f);
}

void Game::update(float deltaTime) {
    camera->update(deltaTime);
}
