#include "GLBLoader.h"
#include <tiny_gltf.h>
#include <iostream>

MeshNode* GLBLoader::loadGLB(const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (!loader.LoadBinaryFromFile(&model, &err, &warn, path)) {
        std::cerr << "Failed to load GLB file: " << path << std::endl;
        return nullptr;
    }

    // Process the loaded model and create MeshNode instances
    // This is a simplified example - you would need to implement the actual mesh processing logic
    return nullptr;
}
