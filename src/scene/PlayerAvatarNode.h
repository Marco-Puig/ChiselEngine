#pragma once

#include "scene/MeshNode.h"

#include <glm/glm.hpp>
#include <string>

class PlayerAvatarNode : public MeshNode {
public:
    PlayerAvatarNode(const std::string& name, const glm::vec3& color);

private:
    void build(const glm::vec3& color);
};