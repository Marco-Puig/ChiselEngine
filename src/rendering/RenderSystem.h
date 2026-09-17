#pragma once
#include <vector>
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
    void traverseAndRender(Node* node, glm::mat4 parentTransform);
};
