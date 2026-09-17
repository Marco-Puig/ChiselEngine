#include "RenderSystem.h"
#include "scene/MeshNode.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

void RenderSystem::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
    m_shader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition;\nuniform mat4 uModel;\nuniform mat4 uView;\nuniform mat4 uProjection;\nvoid main(){gl_Position=uProjection*uView*uModel*vec4(aPosition,1.0);}",
        "#version 450 core\nout vec4 FragColor;\nvoid main(){FragColor=vec4(0.35,0.65,0.95,1.0);}");
}

void RenderSystem::render(Node* rootNode) {
    const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
                                       glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    renderView(rootNode, view, proj, 0, 1280, 720);
}

void RenderSystem::renderView(Node* rootNode, const glm::mat4& view,
                              const glm::mat4& proj, unsigned int framebuffer,
                              int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_shader->use();
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);
    traverseAndRender(rootNode);
}

void RenderSystem::traverseAndRender(Node* node) {
    if (!node) return;

    glm::mat4 worldTransform = node->getWorldTransform();
    
    if (MeshNode* meshNode = dynamic_cast<MeshNode*>(node)) {
        m_shader->setMat4("uModel", worldTransform);
        glBindVertexArray(meshNode->getVAO());
        glDrawElements(GL_TRIANGLES, meshNode->getIndexCount(), GL_UNSIGNED_INT, nullptr);
    }

    for (const auto& child : node->getChildren())
        traverseAndRender(child.get());
}
