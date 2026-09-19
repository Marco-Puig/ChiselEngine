#pragma once

#include "Node.h"
#include <memory>
#include <stdexcept>
#include <utility>

class Scene {
public:
    Scene() : m_root(std::make_unique<Node>("Root")) {}

    Node* getRoot() const { return m_root.get(); }

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
    std::unique_ptr<Node> m_root;
};
