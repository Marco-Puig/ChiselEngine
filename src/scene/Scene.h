#pragma once

#include "Node.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

class Scene {
public:
    Scene() : m_root(std::make_unique<Node>("Root")) {}

    Node* getRoot() const { return m_root.get(); }

    Node* findByName(const std::string& name) const {
        return findByName(*m_root, name);
    }

    template <typename T, typename... Args>
    T* create(Args&&... args) {
        return create(*m_root, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T* create(Node& parent, Args&&... args) {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = node.get();
        parent.addChild(std::move(node));
        return result;
    }

    template <typename T>
    T* adopt(std::unique_ptr<T> node) {
        if (!node)
            return nullptr;
        T* result = node.get();
        if (result->getParent() != nullptr)
            throw std::logic_error("Cannot adopt a node that already has a parent");
        m_root->addChild(std::move(node));
        return result;
    }

private:
    static Node* findByName(Node& node, const std::string& name) {
        if (node.getName() == name)
            return &node;
        for (const auto& child : node.getChildren()) {
            if (Node* result = findByName(*child, name))
                return result;
        }
        return nullptr;
    }

    std::unique_ptr<Node> m_root;
};
