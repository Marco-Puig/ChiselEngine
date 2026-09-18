#include "RenderSystem.h"
#include "scene/MeshNode.h"
#include "Platform/PhysicsSystem.h"
#include "scene/ArcRotateCamera.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

void RenderSystem::init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.08f, 0.1f, 0.14f, 1.0f);
    m_shader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition;\nuniform mat4 uModel;\nuniform mat4 uView;\nuniform mat4 uProjection;\nvoid main(){gl_Position=uProjection*uView*uModel*vec4(aPosition,1.0);}",
        "#version 450 core\nout vec4 FragColor;\nvoid main(){FragColor=vec4(0.35,0.65,0.95,1.0);}");
    m_debugShader = std::make_unique<Shader>(
        "#version 450 core\nlayout(location=0) in vec3 aPosition;\nuniform mat4 uView;\nuniform mat4 uProjection;\nvoid main(){gl_Position=uProjection*uView*vec4(aPosition,1.0);}",
        "#version 450 core\nout vec4 FragColor;\nvoid main(){FragColor=vec4(1.0,0.7,0.1,1.0);}");
    glGenVertexArrays(1, &m_debugVao);
    glGenBuffers(1, &m_debugVbo);
}

void RenderSystem::render(Node* rootNode) {
    const glm::mat4 view = m_desktopCamera != nullptr
        ? m_desktopCamera->getViewMatrix()
        : glm::lookAt(glm::vec3(0.0f, 0.0f, 4.0f),
                      glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj = m_desktopCamera != nullptr
        ? m_desktopCamera->getProjectionMatrix()
        : glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
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
    renderCollisionDebug(view, proj);
}

void RenderSystem::renderCollisionDebug(const glm::mat4& view, const glm::mat4& projection) {
    const std::vector<PhysicsDebugLine> lines = PhysicsSystem::getInstance().getDebugLines();
    if (lines.empty())
        return;
    std::vector<glm::vec3> vertices;
    vertices.reserve(lines.size() * 2);
    for (const PhysicsDebugLine& line : lines) {
        vertices.push_back(line.from);
        vertices.push_back(line.to);
    }
    m_debugShader->use();
    m_debugShader->setMat4("uView", view);
    m_debugShader->setMat4("uProjection", projection);
    glBindVertexArray(m_debugVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_debugVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                 vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glEnableVertexAttribArray(0);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
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
