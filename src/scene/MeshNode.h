#pragma once
#include "Node.h"
#include "rendering/Material.h"
#include <glad/glad.h>
#include <iostream>
#include <vector>

class MeshNode : public Node {
public:
    MeshNode(const std::string& name) : Node(name), m_vao(0), m_vbo(0), m_ebo(0) {}
    ~MeshNode() override {
        GLuint textures[] = {
            m_material.baseColorTexture, m_material.normalTexture,
            m_material.metallicRoughnessTexture, m_material.emissiveTexture};
        glDeleteTextures(4, textures);
        if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
        if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
        if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    }

    void setMesh(unsigned int vao, unsigned int vbo, unsigned int ebo, int indexCount,
                 bool hasNormals = false) {
        m_vao = vao;
        m_vbo = vbo;
        m_ebo = ebo;
        m_indexCount = indexCount;
        m_hasNormals = hasNormals;
    }

    unsigned int getVAO() const { return m_vao; }
    int getIndexCount() const { return m_indexCount; }
    bool hasNormals() const { return m_hasNormals; }
    void setMaterial(const Material& material) { m_material = material; }
    const Material& getMaterial() const { return m_material; }
    void setBounds(const glm::vec3& minimum, const glm::vec3& maximum) {
        m_boundsMin = minimum;
        m_boundsMax = maximum;
    }
    glm::vec3 getBoundsMin() const { return m_boundsMin; }
    glm::vec3 getBoundsMax() const { return m_boundsMax; }
    glm::vec3 getBoundsSize() const { return m_boundsMax - m_boundsMin; }
    void setCollisionVertices(std::vector<glm::vec3> vertices) {
        m_collisionVertices = std::move(vertices);
    }
    const std::vector<glm::vec3>& getCollisionVertices() const {
        return m_collisionVertices;
    }

private:
    unsigned int m_vao, m_vbo, m_ebo;
    int m_indexCount = 0;
    bool m_hasNormals = false;
    glm::vec3 m_boundsMin{0.0f};
    glm::vec3 m_boundsMax{0.0f};
    Material m_material;
    std::vector<glm::vec3> m_collisionVertices;
};
