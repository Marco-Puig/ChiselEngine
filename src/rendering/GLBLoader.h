#pragma once
#include "scene/MeshNode.h"
#include <string>
#include <memory>

class GLBLoader {
public:
    static Node* loadGLB(const std::string& path);
};
