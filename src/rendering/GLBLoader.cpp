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
    std::vector<glm::vec3> positions;
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
        const size_t positionStart = positionView.byteOffset + position.byteOffset;
        const size_t positionStride = stride;
        if (positionStart > positionBuffer.data.size() ||
            position.count > (positionBuffer.data.size() - positionStart) / positionStride)
            throw std::runtime_error("GLB POSITION accessor exceeds its buffer: " + path);
        const unsigned char* data = positionBuffer.data.data() + positionStart;
        for (size_t i = 0; i < position.count; ++i) {
            const float* value = reinterpret_cast<const float*>(data + i * stride);
            positions.emplace_back(value[0], value[1], value[2]);
            boundsMin = glm::min(boundsMin, glm::vec3(value[0], value[1], value[2]));
            boundsMax = glm::max(boundsMax, glm::vec3(value[0], value[1], value[2]));
        }

        if (primitive.indices < 0)
            throw std::runtime_error("GLB primitive has no index buffer: " + path);
        const tinygltf::Accessor& index = model.accessors.at(primitive.indices);
        const tinygltf::BufferView& indexView = model.bufferViews.at(index.bufferView);
        const tinygltf::Buffer& indexBuffer = model.buffers.at(indexView.buffer);
        const size_t indexElementSize =
            index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? sizeof(unsigned char) :
            index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? sizeof(unsigned short) :
            index.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ? sizeof(unsigned int) : 0;
        if (indexElementSize == 0)
            throw std::runtime_error("Unsupported GLB index format: " + path);
        const size_t indexStart = indexView.byteOffset + index.byteOffset;
        if (indexStart > indexBuffer.data.size() ||
            index.count > (indexBuffer.data.size() - indexStart) / indexElementSize)
            throw std::runtime_error("GLB index accessor exceeds its buffer: " + path);
        const unsigned char* indexData = indexBuffer.data.data() + indexStart;
        const unsigned int vertexOffset = static_cast<unsigned int>(positions.size() - position.count);
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
            if (value >= position.count)
                throw std::runtime_error("GLB index references a vertex outside its primitive: " + path);
            indices.push_back(value + vertexOffset);
        }
    }
    if (positions.empty() || indices.empty())
        throw std::runtime_error("GLB mesh has no POSITION/index data: " + path);

    std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        if (indices[i] >= positions.size() ||
            indices[i + 1] >= positions.size() ||
            indices[i + 2] >= positions.size())
            throw std::runtime_error("GLB index references a vertex outside the mesh: " + path);
        const glm::vec3 edgeA = positions[indices[i + 1]] - positions[indices[i]];
        const glm::vec3 edgeB = positions[indices[i + 2]] - positions[indices[i]];
        const glm::vec3 normal = glm::cross(edgeA, edgeB);
        normals[indices[i]] += normal;
        normals[indices[i + 1]] += normal;
        normals[indices[i + 2]] += normal;
    }
    for (size_t i = 0; i < positions.size(); ++i) {
        const glm::vec3 normal = glm::length(normals[i]) > 0.0f
            ? glm::normalize(normals[i]) : glm::vec3(0.0f, 1.0f, 0.0f);
        vertices.insert(vertices.end(), {positions[i].x, positions[i].y, positions[i].z,
                                         normal.x, normal.y, normal.z});
    }

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    auto* mesh = new MeshNode("GLB:" + path);
    mesh->setMesh(vao, vbo, ebo, static_cast<int>(indices.size()), true);
    mesh->setBounds(boundsMin, boundsMax);
    return mesh;
}
