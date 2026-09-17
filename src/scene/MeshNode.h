#pragma once
#include "Node.h"
#include <glad/glad.h>
#include <iostream>

class MeshNode : public Node {
public:
    MeshNode(const std::string& name) : Node(name), m_vao(0), m_vbo(0), m_ebo(0) {}
    ~MeshNode() override {
        if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);
        if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
        if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    }

    void setMesh(unsigned int vao, unsigned int vbo, unsigned int ebo, int indexCount) {
        m_vao = vao;
        m_vbo = vbo;
        m_ebo = ebo;
        m_indexCount = indexCount;
    }

    unsigned int getVAO() const { return m_vao; }
    int getIndexCount() const { return m_indexCount; }

private:
    unsigned int m_vao, m_vbo, m_ebo;
    int m_indexCount = 0;
    // Reference to Material/Shader will be added in Step 4
};
