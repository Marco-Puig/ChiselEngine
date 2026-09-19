#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Node {
public:
    Node(const std::string& name) : m_name(name), m_position(0.0f), m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), m_scale(1.0f), m_parent(nullptr) {}
    virtual ~Node() = default;

    void addChild(std::unique_ptr<Node> child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
    }

    glm::mat4 getWorldTransform() const {
        glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), m_position) * 
                                   glm::mat4_cast(m_rotation) * 
                                   glm::scale(glm::mat4(1.0f), m_scale);
        
        if (m_parent) {
            return m_parent->getWorldTransform() * localTransform;
        }
        return localTransform;
    }

    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    void setPosition(glm::vec3 pos) { m_position = pos; }
    glm::vec3 getPosition() const { return m_position; }
    void setRotation(const glm::quat& rotation) { m_rotation = rotation; }
    const glm::quat& getRotation() const { return m_rotation; }
    void setScale(glm::vec3 scale) { m_scale = scale; }
    glm::vec3 getScale() const { return m_scale; }
    const std::vector<std::unique_ptr<Node>>& getChildren() const { return m_children; }
    Node* getParent() const { return m_parent; }
    void setEditorManipulated(bool manipulated) { m_editorManipulated = manipulated; }
    bool isEditorManipulated() const { return m_editorManipulated; }

protected:
    std::string m_name;
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;
    Node* m_parent;
    bool m_editorManipulated = false;
    std::vector<std::unique_ptr<Node>> m_children;
};
