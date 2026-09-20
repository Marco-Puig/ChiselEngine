#pragma once
#include "scene/MeshNode.h"
#include <string>
#include <memory>

class GLBLoader {
public:
    static MeshNode* loadGLB(const std::string& path);
};
