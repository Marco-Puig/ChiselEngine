#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>

// Using the "Node" terminology similar to Babylon.js
class Node {
public:
    Node(const std::string& name) : m_name(name) {}
    virtual ~Node() = default;

    void setPosition(glm::vec3 pos) { m_position = pos; }
    glm::vec3 getPosition() const { return m_position; }

    void setRotation(glm::quat rot) { m_rotation = rot; }
    glm::quat getRotation() const { return m_rotation; }

    void setScale(glm::vec3 scale) { m_scale = scale; }
    glm::vec3 getScale() const { return m_scale; }

    const std::string& getName() const { return m_name; }

protected:
    std::string m_name;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);
};

class MeshNode : public Node {
public:
    MeshNode(const std::string& name) : Node(name) {}
    // Mesh specific data like VAO/VBO handles would go here
};
