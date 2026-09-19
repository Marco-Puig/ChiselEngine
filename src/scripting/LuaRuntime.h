#pragma once

#include <memory>
#include <string>

struct lua_State;

namespace luabridge {
class LuaRef;
}

class Animator;
class Node;
class Scene;

class LuaRuntime {
public:
    LuaRuntime();
    ~LuaRuntime();

    LuaRuntime(const LuaRuntime&) = delete;
    LuaRuntime& operator=(const LuaRuntime&) = delete;

    bool initialize();
    void shutdown();
    bool isInitialized() const { return m_state != nullptr; }

    bool loadGameScript(const std::string& path);
    void startGame(Scene& scene, Animator& animator);
    void updateGame(float deltaTime);
    lua_State* state() const { return m_state; }

private:
    void bindEngineApi();

    lua_State* m_state = nullptr;
    std::string m_scriptPath;
    std::unique_ptr<luabridge::LuaRef> m_gameScript;
    Scene* m_scene = nullptr;
    Animator* m_animator = nullptr;
    bool m_started = false;
};
