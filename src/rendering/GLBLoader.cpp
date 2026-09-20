#include "GLBLoader.h"
#include "Material.h"
#include <glad/glad.h>
#include <tiny_gltf.h>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
struct SceneMeshInstance {
    int mesh = -1;
    int nodeIndex = -1;
    glm::mat4 transform{1.0f};
};

glm::mat4 nodeTransform(const tinygltf::Node& node) {
    if (node.matrix.size() == 16) {
        glm::mat4 result(1.0f);
        for (int column = 0; column < 4; ++column)
            for (int row = 0; row < 4; ++row)
                result[column][row] =
                    static_cast<float>(node.matrix[column * 4 + row]);
        return result;
    }

    glm::vec3 translation(0.0f);
    if (node.translation.size() == 3)
        translation = glm::vec3(static_cast<float>(node.translation[0]),
                                static_cast<float>(node.translation[1]),
                                static_cast<float>(node.translation[2]));

    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    if (node.rotation.size() == 4)
        rotation = glm::quat(static_cast<float>(node.rotation[3]),
                             static_cast<float>(node.rotation[0]),
                             static_cast<float>(node.rotation[1]),
                             static_cast<float>(node.rotation[2]));

    glm::vec3 scale(1.0f);
    if (node.scale.size() == 3)
        scale = glm::vec3(static_cast<float>(node.scale[0]),
                          static_cast<float>(node.scale[1]),
                          static_cast<float>(node.scale[2]));

    return glm::translate(glm::mat4(1.0f), translation) *
           glm::mat4_cast(rotation) *
           glm::scale(glm::mat4(1.0f), scale);
}

void collectSceneMeshInstances(const tinygltf::Model& model, int nodeIndex,
                                const glm::mat4& parentTransform,
                                std::vector<SceneMeshInstance>& instances) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size())) {
        std::cerr << "GLB scene references invalid node index " << nodeIndex << '\n';
        return;
    }
    const tinygltf::Node& node = model.nodes[nodeIndex];
    const glm::mat4 worldTransform = parentTransform * nodeTransform(node);
    if (node.mesh >= 0)
        instances.push_back({node.mesh, nodeIndex, worldTransform});
    for (int child : node.children)
        collectSceneMeshInstances(model, child, worldTransform, instances);
}

void collectDefaultSceneMeshInstances(const tinygltf::Model& model,
                                      std::vector<SceneMeshInstance>& instances) {
    if (model.defaultScene >= 0 &&
        model.defaultScene < static_cast<int>(model.scenes.size())) {
        for (int node : model.scenes[model.defaultScene].nodes)
            collectSceneMeshInstances(model, node, glm::mat4(1.0f), instances);
    }
}

std::vector<glm::vec3> readMeshPositions(const tinygltf::Model& model,
                                         int meshIndex,
                                         const std::string& path) {
    std::vector<glm::vec3> result;
    if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size()))
        return result;
    for (const auto& primitive : model.meshes[meshIndex].primitives) {
        const auto positionIt = primitive.attributes.find("POSITION");
        if (positionIt == primitive.attributes.end() ||
            positionIt->second < 0 ||
            positionIt->second >= static_cast<int>(model.accessors.size()))
            continue;
        const tinygltf::Accessor& accessor = model.accessors[positionIt->second];
        if (accessor.bufferView < 0 ||
            accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
            continue;
        const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
        if (view.buffer < 0 ||
            view.buffer >= static_cast<int>(model.buffers.size()))
            continue;
        const tinygltf::Buffer& buffer = model.buffers[view.buffer];
        const size_t stride = accessor.ByteStride(view) != 0
            ? accessor.ByteStride(view) : sizeof(float) * 3;
        const size_t start = view.byteOffset + accessor.byteOffset;
        if (stride < sizeof(float) * 3 ||
            start > buffer.data.size() ||
            accessor.count > (buffer.data.size() - start) / stride) {
            std::cerr << "GLB POSITION accessor exceeds its buffer: "
                      << path << '\n';
            continue;
        }
        const unsigned char* data = buffer.data.data() + start;
        for (size_t i = 0; i < accessor.count; ++i) {
            const float* value =
                reinterpret_cast<const float*>(data + i * stride);
            result.emplace_back(value[0], value[1], value[2]);
        }
    }
    return result;
}

GLuint createTexture(const tinygltf::Image& image, bool srgb) {
    if (image.image.empty() || image.width <= 0 || image.height <= 0)
        return 0;
    GLenum format = image.component == 4 ? GL_RGBA : image.component == 3 ? GL_RGB : GL_RED;
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, srgb && format == GL_RGB ? GL_SRGB8 :
                 srgb && format == GL_RGBA ? GL_SRGB8_ALPHA8 : format,
                 image.width, image.height, 0, format, GL_UNSIGNED_BYTE,
                 image.image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    return texture;
}

int materialTexture(const tinygltf::Model& model, int textureIndex,
                    bool srgb) {
    if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size()))
        return 0;
    const int imageIndex = model.textures[textureIndex].source;
    if (imageIndex < 0 || imageIndex >= static_cast<int>(model.images.size()))
        return 0;
    return static_cast<int>(createTexture(model.images[imageIndex], srgb));
}

void collectSceneMeshes(const tinygltf::Model& model, int nodeIndex,
                        std::vector<int>& meshes) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(model.nodes.size())) {
        std::cerr << "GLB scene references invalid node index " << nodeIndex << '\n';
        return;
    }
    const tinygltf::Node& node = model.nodes[nodeIndex];
    if (node.mesh >= 0)
        meshes.push_back(node.mesh);
    for (int child : node.children)
        collectSceneMeshes(model, child, meshes);
}

int findPrimarySceneMesh(const tinygltf::Model& model) {
    std::vector<int> sceneMeshes;
    if (model.defaultScene >= 0 &&
        model.defaultScene < static_cast<int>(model.scenes.size())) {
        for (int node : model.scenes[model.defaultScene].nodes)
            collectSceneMeshes(model, node, sceneMeshes);
    }
    if (sceneMeshes.empty()) {
        for (size_t i = 0; i < model.meshes.size(); ++i)
            sceneMeshes.push_back(static_cast<int>(i));
        std::cerr << "GLB has no mesh nodes in its default scene; checking all meshes\n";
    }

    int selected = -1;
    size_t largestVertexCount = 0;
    for (int meshIndex : sceneMeshes) {
        if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) {
            std::cerr << "GLB scene references invalid mesh index " << meshIndex << '\n';
            continue;
        }
        size_t vertexCount = 0;
        for (const auto& primitive : model.meshes[meshIndex].primitives) {
            const auto position = primitive.attributes.find("POSITION");
            if (position != primitive.attributes.end() &&
                position->second >= 0 &&
                position->second < static_cast<int>(model.accessors.size()))
                vertexCount += model.accessors[position->second].count;
        }
        std::cerr << "GLB scene mesh " << meshIndex << " ("
                  << model.meshes[meshIndex].name << ") has "
                  << vertexCount << " vertices\n";
        if (vertexCount > largestVertexCount) {
            largestVertexCount = vertexCount;
            selected = meshIndex;
        }
    }
    return selected;
}
}

Node* GLBLoader::loadGLB(const std::string& path) {
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
    if (!warn.empty())
        std::cerr << "GLB warning for '" << path << "': " << warn << '\n';

    if (model.meshes.empty())
        throw std::runtime_error("GLB contains no renderable mesh: " + path);

    std::vector<SceneMeshInstance> sceneInstances;
    collectDefaultSceneMeshInstances(model, sceneInstances);
    if (sceneInstances.empty()) {
        for (size_t i = 0; i < model.meshes.size(); ++i)
            sceneInstances.push_back({static_cast<int>(i), static_cast<int>(i), glm::mat4(1.0f)});
        std::cerr << "GLB has no mesh nodes in its default scene; checking all meshes\n";
    }

    auto* root = new Node("GLB_Root:" + path);

    for (const auto& instance : sceneInstances) {
        const int meshIndex = instance.mesh;
        if (meshIndex < 0 || meshIndex >= static_cast<int>(model.meshes.size())) {
            std::cerr << "GLB scene references invalid mesh index " << meshIndex << '\n';
            continue;
        }
        const tinygltf::Mesh& sourceMesh = model.meshes[meshIndex];
        if (sourceMesh.primitives.empty())
            continue;

        glm::mat4 worldTransform = instance.transform;
        glm::vec3 pos = glm::vec3(worldTransform[3]);
        glm::quat rot = glm::quat_cast(worldTransform);
        glm::vec3 scale = glm::vec3(glm::length(worldTransform[0]), glm::length(worldTransform[1]), glm::length(worldTransform[2]));

        std::vector<float> vertices;
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> sourceNormals;
        std::vector<glm::vec2> texcoords;
        int firstMaterialIndex = -1;
        std::vector<unsigned int> indices;
        glm::vec3 boundsMin(std::numeric_limits<float>::max());
        glm::vec3 boundsMax(std::numeric_limits<float>::lowest());

        for (const auto& primitive : sourceMesh.primitives) {
            if (firstMaterialIndex < 0)
                firstMaterialIndex = primitive.material;
            const auto positionIt = primitive.attributes.find("POSITION");
            if (positionIt == primitive.attributes.end())
                continue;
            if (positionIt->second < 0 || positionIt->second >= static_cast<int>(model.accessors.size()))
                continue;
            const tinygltf::Accessor& position = model.accessors[positionIt->second];
            if (position.bufferView < 0 || position.bufferView >= static_cast<int>(model.bufferViews.size()))
                continue;
            const tinygltf::BufferView& positionView = model.bufferViews[position.bufferView];
            if (positionView.buffer < 0 || positionView.buffer >= static_cast<int>(model.buffers.size()))
                continue;
            const tinygltf::Buffer& positionBuffer = model.buffers[positionView.buffer];
            const size_t stride = position.ByteStride(positionView) != 0 ? position.ByteStride(positionView) : sizeof(float) * 3;
            const size_t positionStart = positionView.byteOffset + position.byteOffset;
            const unsigned char* data = positionBuffer.data.data() + positionStart;
            for (size_t i = 0; i < position.count; ++i) {
                const float* value = reinterpret_cast<const float*>(data + i * stride);
                const glm::vec3 localPos = glm::vec3(value[0], value[1], value[2]);
                positions.push_back(localPos);
                boundsMin = glm::min(boundsMin, localPos);
                boundsMax = glm::max(boundsMax, localPos);
            }

            const auto normalIt = primitive.attributes.find("NORMAL");
            if (normalIt != primitive.attributes.end()) {
                const tinygltf::Accessor& normal = model.accessors.at(normalIt->second);
                const tinygltf::BufferView& view = model.bufferViews.at(normal.bufferView);
                const tinygltf::Buffer& buffer = model.buffers.at(view.buffer);
                const size_t normalStride = normal.ByteStride(view) != 0 ? normal.ByteStride(view) : sizeof(float) * 3;
                const unsigned char* normalData = buffer.data.data() + view.byteOffset + normal.byteOffset;
                for (size_t i = 0; i < normal.count; ++i) {
                    const float* value = reinterpret_cast<const float*>(normalData + i * normalStride);
                    sourceNormals.push_back(glm::vec3(value[0], value[1], value[2]));
                }
            } else {
                sourceNormals.resize(positions.size(), glm::vec3(0.0f));
            }
            const auto uvIt = primitive.attributes.find("TEXCOORD_0");
            if (uvIt != primitive.attributes.end()) {
                const tinygltf::Accessor& uv = model.accessors.at(uvIt->second);
                const tinygltf::BufferView& view = model.bufferViews.at(uv.bufferView);
                const tinygltf::Buffer& buffer = model.buffers.at(view.buffer);
                const size_t uvStride = uv.ByteStride(view) != 0 ? uv.ByteStride(view) : sizeof(float) * 2;
                const unsigned char* uvData = buffer.data.data() + view.byteOffset + uv.byteOffset;
                for (size_t i = 0; i < uv.count; ++i) {
                    const float* value = reinterpret_cast<const float*>(uvData + i * uvStride);
                    texcoords.emplace_back(value[0], value[1]);
                }
            } else {
                texcoords.resize(positions.size(), glm::vec2(0.0f));
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
            continue;

        std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            if (indices[i] >= positions.size() || indices[i + 1] >= positions.size() || indices[i + 2] >= positions.size())
                throw std::runtime_error("GLB index references a vertex outside the mesh: " + path);
            const glm::vec3 edgeA = positions[indices[i + 1]] - positions[indices[i]];
            const glm::vec3 edgeB = positions[indices[i + 2]] - positions[indices[i]];
            const glm::vec3 normal = glm::cross(edgeA, edgeB);
            normals[indices[i]] += normal;
            normals[indices[i + 1]] += normal;
            normals[indices[i + 2]] += normal;
        }
        for (size_t i = 0; i < positions.size(); ++i) {
            const glm::vec3 normal = glm::length(normals[i]) > 0.0f ? glm::normalize(normals[i]) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 sourceNormal = i < sourceNormals.size() ? sourceNormals[i] : normal;
            const glm::vec3 finalNormal = glm::length(sourceNormal) > 0.0f ? glm::normalize(sourceNormal) : normal;
            const glm::vec2 uv = i < texcoords.size() ? texcoords[i] : glm::vec2(0.0f);
            vertices.insert(vertices.end(), {positions[i].x, positions[i].y, positions[i].z,
                                            finalNormal.x, finalNormal.y, finalNormal.z,
                                            uv.x, uv.y});
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
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        auto* meshNode = new MeshNode("GLB_Mesh:" + path);
        
        if (instance.nodeIndex >= 0 && instance.nodeIndex < static_cast<int>(model.nodes.size())) {
            const std::string& nodeName = model.nodes[instance.nodeIndex].name;
            if (!nodeName.empty()) {
                meshNode->setName(nodeName);
            } else {
                meshNode->setName("node_" + std::to_string(instance.nodeIndex));
            }
        }
        
        meshNode->setMesh(vao, vbo, ebo, static_cast<int>(indices.size()), true);
        meshNode->setBounds(boundsMin, boundsMax);
        meshNode->setPosition(pos);
        meshNode->setRotation(rot);
        meshNode->setScale(scale);
    
        std::vector<glm::vec3> subCollisionVertices;
        const std::vector<glm::vec3> readPositions = readMeshPositions(model, meshIndex, path);
        for (const glm::vec3& point : readPositions) {
            const glm::vec3 transformed = glm::vec3(worldTransform * glm::vec4(point, 1.0f));
            subCollisionVertices.push_back(transformed);
        }
        meshNode->setCollisionVertices(std::move(subCollisionVertices));
    
        // Resolve material for this specific primitive
        if (sourceMesh.primitives.empty()) {
            std::cerr << "GLB mesh " << meshIndex << " has no primitives\n";
        } else {
            // Use the material of the first primitive for the MeshNode
            int matIndex = sourceMesh.primitives[0].material;
            if (matIndex >= 0 && matIndex < static_cast<int>(model.materials.size())) {
                const tinygltf::Material& source = model.materials[matIndex];
                Material material;
                const auto& pbr = source.pbrMetallicRoughness;
                material.baseColorFactor = glm::vec4(
                    static_cast<float>(pbr.baseColorFactor[0]),
                    static_cast<float>(pbr.baseColorFactor[1]),
                    static_cast<float>(pbr.baseColorFactor[2]),
                    static_cast<float>(pbr.baseColorFactor[3]));
                material.metallicFactor = static_cast<float>(pbr.metallicFactor);
                material.roughnessFactor = static_cast<float>(pbr.roughnessFactor);
                material.baseColorTexture = materialTexture(model, pbr.baseColorTexture.index, true);
                material.metallicRoughnessTexture = materialTexture(model, pbr.metallicRoughnessTexture.index, false);
                material.normalTexture = materialTexture(model, source.normalTexture.index, false);
                material.emissiveTexture = materialTexture(model, source.emissiveTexture.index, true);
                meshNode->setMaterial(material);
                
                std::cout << "[GLB] Loaded " << meshNode->getName() << " with material " << matIndex 
                          << " (Color: " << material.baseColorFactor.r << ", " << material.baseColorFactor.g 
                          << ", " << material.baseColorFactor.b << ")\n";
            } else {
                std::cout << "[GLB] " << meshNode->getName() << " uses default material\n";
            }
        }
        root->addChild(std::unique_ptr<MeshNode>(meshNode));

    }

    return root;
}
