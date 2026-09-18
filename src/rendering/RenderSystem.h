#pragma once
#include <vector>
#include <memory>
#include <string>
#include "scene/Node.h"
#include "rendering/Light.h"
#include "rendering/Shader.h"
#include <glm/glm.hpp>

struct PhysicsDebugLine;
class ArcRotateCamera;
class DirectionalLight;

class RenderSystem {
public:
    static RenderSystem& getInstance() {
        static RenderSystem instance;
        return instance;
    }

    void init();
    void render(Node* rootNode);
    void renderView(Node* rootNode, const glm::mat4& view,
                    const glm::mat4& projection, unsigned int framebuffer,
                    int width, int height);
    void addLight(Light* light) { m_lights.push_back(light); }
    void setDesktopCamera(ArcRotateCamera* camera) { m_desktopCamera = camera; }
    void setSkyboxPath(const std::string& path) { m_skyboxPath = path; }

private:
    RenderSystem() = default;
    std::vector<Light*> m_lights;
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Shader> m_debugShader;
    std::unique_ptr<Shader> m_skyboxShader;
    unsigned int m_debugVao = 0;
    unsigned int m_debugVbo = 0;
    unsigned int m_skyboxVao = 0;
    unsigned int m_skyboxVbo = 0;
    unsigned int m_skyboxTexture = 0;
    std::string m_skyboxPath;
    ArcRotateCamera* m_desktopCamera = nullptr;
    void traverseAndRender(Node* node);
    void renderCollisionDebug(const glm::mat4& view, const glm::mat4& projection);
    void renderSkybox(const glm::mat4& view, const glm::mat4& projection);
};
