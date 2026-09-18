#include "GLBLoader.h"
#include <glad/glad.h>
#include <tiny_gltf.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <limits>
#include <glm/glm.hpp>

MeshNode* GLBLoader::loadGLB(const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open GLB file: " + path);
    const std::vector<unsigned char> input((std::istreambuf_iterator<char>(file)),
                                           std::istreambuf_iterator<char>());
    if (input.size() < 20)
        throw std::runtime_error("GLB file is truncated: " + path);

    // Accept older exporters that omitted the required JSON chunk padding.
    std::vector<unsigned char> normalized = input;
    const uint32_t jsonLength = input[12] | (input[13] << 8) | (input[14] << 16) | (input[15] << 24);
    const uint32_t padding = (4 - (jsonLength % 4)) % 4;
    if (padding != 0) {
        normalized.insert(normalized.begin() + 20 + jsonLength, padding, static_cast<unsigned char>(' '));
        const uint32_t paddedJsonLength = jsonLength + padding;
        const uint32_t totalLength = static_cast<uint32_t>(normalized.size());
        for (unsigned int i = 0; i < 4; ++i) {
            normalized[12 + i] = static_cast<unsigned char>((paddedJsonLength >> (8 * i)) & 0xff);
            normalized[8 + i] = static_cast<unsigned char>((totalLength >> (8 * i)) & 0xff);
        }
    }

    if (!loader.LoadBinaryFromMemory(&model, &err, &warn, normalized.data(),
                                     static_cast<unsigned int>(normalized.size())))
        throw std::runtime_error("Failed to load GLB file '" + path + "': " + err);

    if (model.meshes.empty() || model.meshes.front().primitives.empty())
        throw std::runtime_error("GLB contains no renderable mesh: " + path);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    glm::vec3 boundsMin(std::numeric_limits<float>::max());
    glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
    for (const auto& primitive : model.meshes.front().primitives) {
        const auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end())
            continue;
        const tinygltf::Accessor& position = model.accessors.at(positionIt->second);
        const tinygltf::BufferView& positionView = model.bufferViews.at(position.bufferView);
        const tinygltf::Buffer& positionBuffer = model.buffers.at(positionView.buffer);
        const size_t stride = position.ByteStride(positionView) != 0
            ? position.ByteStride(positionView)
            : sizeof(float) * 3;
        const unsigned char* data = positionBuffer.data.data() + positionView.byteOffset + position.byteOffset;
        for (size_t i = 0; i < position.count; ++i) {
            const float* value = reinterpret_cast<const float*>(data + i * stride);
            vertices.insert(vertices.end(), value, value + 3);
            boundsMin = glm::min(boundsMin, glm::vec3(value[0], value[1], value[2]));
            boundsMax = glm::max(boundsMax, glm::vec3(value[0], value[1], value[2]));
        }

        if (primitive.indices < 0)
            throw std::runtime_error("GLB primitive has no index buffer: " + path);
        const tinygltf::Accessor& index = model.accessors.at(primitive.indices);
        const tinygltf::BufferView& indexView = model.bufferViews.at(index.bufferView);
        const tinygltf::Buffer& indexBuffer = model.buffers.at(indexView.buffer);
        const unsigned char* indexData = indexBuffer.data.data() + indexView.byteOffset + index.byteOffset;
        const unsigned int vertexOffset = static_cast<unsigned int>(vertices.size() / 3 - position.count);
        for (size_t i = 0; i < index.count; ++i) {
            unsigned int value = 0;
            if (index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                value = indexData[i];
            else if (index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                value = reinterpret_cast<const unsigned short*>(indexData)[i];
            else if (index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                value = reinterpret_cast<const unsigned int*>(indexData)[i];
            else
                throw std::runtime_error("Unsupported GLB index format: " + path);
            indices.push_back(value + vertexOffset);
        }
    }
    if (vertices.empty() || indices.empty())
        throw std::runtime_error("GLB mesh has no POSITION/index data: " + path);

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    auto* mesh = new MeshNode("GLB:" + path);
    mesh->setMesh(vao, vbo, ebo, static_cast<int>(indices.size()));
    mesh->setBounds(boundsMin, boundsMax);
    return mesh;
}
