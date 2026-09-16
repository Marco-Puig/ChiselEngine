#include "Rendering/RenderSystem.h"
#include "Scene/Node.h"
#include <vector>

void RenderSystem::renderScene() {
    // 1. Update global lighting uniforms once per frame
    updateLightingUniforms(m_appShaderProgram);

    // 2. In a full implementation, we would iterate through a Scene object's nodes
    // and call drawNode() for each MeshNode.
    // Example:
    // for (auto node : currentScene->getNodes()) {
    //     drawNode(node);
    // }
}
