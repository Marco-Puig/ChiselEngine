#include "LuaRuntime.h"

#include "scene/Animator.h"
#include "scene/Node.h"
#include "scene/Scene.h"
#include "rendering/GLBLoader.h"
#include "rendering/Light.h"
#include "rendering/RenderSystem.h"
#include "scene/MeshNode.h"
#include "platform/PhysicsSystem.h"

#include <lua.hpp>
#include <LuaBridge/LuaBridge.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

namespace {
void setPosition(Node* node, float x, float y, float z) {
    if (node != nullptr)
        node->setPosition(glm::vec3(x, y, z));
}

void setScale(Node* node, float x, float y, float z) {
    if (node != nullptr)
        node->setScale(glm::vec3(x, y, z));
}

void setLightPosition(DirectionalLight* light, float x, float y, float z) {
    setPosition(light, x, y, z);
}

void setLightColor(DirectionalLight* light, float r, float g, float b) {
    if (light != nullptr)
        light->setColor(glm::vec3(r, g, b));
}

Node* loadMesh(Scene* scene, const std::string& path,
               const std::string& name) {
    if (scene == nullptr)
        return nullptr;
    Node* root = GLBLoader::loadGLB(path);
    if (root == nullptr)
        return nullptr;
    root->setName(name);
    return scene->adopt(std::unique_ptr<Node>(root));
}

DirectionalLight* createDirectionalLight(Scene* scene,
                                          const std::string& name) {
    if (scene == nullptr)
        return nullptr;
    auto light = std::make_unique<DirectionalLight>(
        name, glm::vec3(-0.2f, -1.0f, -0.3f));
    DirectionalLight* result = light.get();
    scene->adopt(std::move(light));
    RenderSystem::getInstance().addLight(result);
    return result;
}

bool addRigidBody(Node* node, const std::string& type,
                  float friction, float restitution) {
    if (node == nullptr)
        return false;
    auto* mesh = dynamic_cast<MeshNode*>(node);
    if (mesh == nullptr)
        return false;
    BodyType bodyType = BodyType::Static;
    if (type == "dynamic")
        bodyType = BodyType::Dynamic;
    else if (type == "kinematic")
        bodyType = BodyType::Kinematic;
    return PhysicsSystem::getInstance().createRigidBody(
        node, bodyType, glm::max(mesh->getBoundsSize(), glm::vec3(0.01f)),
        friction, restitution) != nullptr;
}

void setSkybox(const std::string& path) {
    RenderSystem::getInstance().setSkyboxPath(path);
}

float getPositionX(Node* node) { return node ? node->getPosition().x : 0.0f; }
float getPositionY(Node* node) { return node ? node->getPosition().y : 0.0f; }
float getPositionZ(Node* node) { return node ? node->getPosition().z : 0.0f; }
}

LuaRuntime::LuaRuntime() = default;

LuaRuntime::~LuaRuntime() {
    shutdown();
}

bool LuaRuntime::initialize() {
    if (m_state != nullptr)
        return true;

    m_state = luaL_newstate();
    if (m_state == nullptr) {
        std::cerr << "[Lua] Failed to create Lua state\n";
        return false;
    }

    luaL_openlibs(m_state);
    bindEngineApi();
    return true;
}

void LuaRuntime::shutdown() {
    if (m_state != nullptr) {
        m_gameScript.reset();
        lua_close(m_state);
        m_state = nullptr;
    }
    m_scene = nullptr;
    m_animator = nullptr;
    m_started = false;
}

void LuaRuntime::bindEngineApi() {
    luabridge::getGlobalNamespace(m_state)
        .beginClass<Animator>("Animator")
            .addFunction("animateAxis", &Animator::animateAxis)
            .addFunction("procedural", &Animator::proceduralFromLua)
        .endClass()
        .beginClass<Scene>("Scene")
            .addFunction("findNode", &Scene::findByName)
            .addFunction("loadMesh", &loadMesh)
            .addFunction("createDirectionalLight", &createDirectionalLight)
        .endClass();
    luabridge::getGlobalNamespace(m_state)
        .beginClass<Node>("Node")
            .addFunction("getName", &Node::getName)
            .addFunction("setName", &Node::setName)
            .addFunction("setPosition", &setPosition)
            .addFunction("getPositionX", &getPositionX)
            .addFunction("getPositionY", &getPositionY)
            .addFunction("getPositionZ", &getPositionZ)
            .addFunction("setScale", &setScale)
        .endClass()
        .beginClass<DirectionalLight>("DirectionalLight")
            .addFunction("setPosition", &setLightPosition)
            .addFunction("setColor", &setLightColor)
            .addFunction("setIntensity", &DirectionalLight::setIntensity)
            .addFunction("setExposure", &DirectionalLight::setExposure)
        .endClass();
    luabridge::getGlobalNamespace(m_state)
        .beginNamespace("Engine")
            .addFunction("addRigidBody", &addRigidBody)
            .addFunction("setSkybox", &setSkybox)
        .endNamespace();
}

bool LuaRuntime::loadGameScript(const std::string& path) {
    if (m_state == nullptr) {
        std::cerr << "[Lua] Cannot load '" << path
                  << "': runtime is not initialized\n";
        return false;
    }

    if (luaL_loadfile(m_state, path.c_str()) != LUA_OK) {
        std::cerr << "[Lua] Load error in '" << path << "': "
                  << lua_tostring(m_state, -1) << '\n';
        lua_pop(m_state, 1);
        return false;
    }
    if (lua_pcall(m_state, 0, 1, 0) != LUA_OK) {
        std::cerr << "[Lua] Startup error in '" << path << "': "
                  << lua_tostring(m_state, -1) << '\n';
        lua_pop(m_state, 1);
        return false;
    }

    if (!lua_istable(m_state, -1)) {
        std::cerr << "[Lua] Script '" << path
                  << "' must return a table\n";
        lua_pop(m_state, 1);
        return false;
    }

    m_gameScript = std::make_unique<luabridge::LuaRef>(
        luabridge::LuaRef::fromStack(m_state, -1));
    lua_pop(m_state, 1);
    m_scriptPath = path;
    m_started = false;
    return true;
}

void LuaRuntime::startGame(Scene& scene, Animator& animator) {
    if (m_started || !m_gameScript)
        return;
    m_scene = &scene;
    m_animator = &animator;
    try {
        luabridge::LuaRef callback = (*m_gameScript)["onStart"];
        if (callback.isFunction())
            callback(&scene, &animator);
        m_started = true;
    } catch (const std::exception& error) {
        std::cerr << "[Lua] onStart error in '" << m_scriptPath << "': "
                  << error.what() << '\n';
    }
}

void LuaRuntime::updateGame(float deltaTime) {
    if (!m_started || !m_gameScript || m_scene == nullptr ||
        m_animator == nullptr)
        return;
    try {
        luabridge::LuaRef callback = (*m_gameScript)["onUpdate"];
        if (callback.isFunction())
            callback(deltaTime, m_scene, m_animator);
    } catch (const std::exception& error) {
        std::cerr << "[Lua] onUpdate error in '" << m_scriptPath << "': "
                  << error.what() << '\n';
    }
}
