#pragma once
#include <vector>
#include <memory>
#include "scene/Node.h"
#include "rendering/Light.h"
#include "rendering/Shader.h"

class RenderSystem {
public:
    static RenderSystem& getInstance() {
        static RenderSystem instance;
        return instance;
    }

    void init();
    void render(Node* rootNode);
    void addLight(Light* light) { m_lights.push_back(light); }

private:
    RenderSystem() = default;
    std::vector<Light*> m_lights;
    std::unique_ptr<Shader> m_shader;
    void traverseAndRender(Node* node);
};
