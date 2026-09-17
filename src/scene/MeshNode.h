#pragma once
#include "Node.h"
#include <iostream>

class MeshNode : public Node {
public:
    MeshNode(const std::string& name) : Node(name), m_vao(0), m_vbo(0), m_ebo(0) {}

    void setMesh(unsigned int vao, unsigned int vbo, unsigned int ebo) {
        m_vao = vao;
        m_vbo = vbo;
        m_ebo = ebo;
    }

    unsigned int getVAO() const { return m_vao; }

private:
    unsigned int m_vao, m_vbo, m_ebo;
    // Reference to Material/Shader will be added in Step 4
};
