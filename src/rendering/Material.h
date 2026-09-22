#pragma once
#include <glm/glm.hpp>

struct Material {
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec3 emissiveFactor{0.0f, 0.0f, 0.0f};
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;

    unsigned int baseColorTexture = 0;
    unsigned int metallicRoughnessTexture = 0;
    unsigned int normalTexture = 0;
    unsigned int occlusionTexture = 0;
    unsigned int emissiveTexture = 0;

    bool hasBaseColorTexture() const { return baseColorTexture != 0; }
    bool hasMetallicRoughnessTexture() const { return metallicRoughnessTexture != 0; }
    bool hasNormalTexture() const { return normalTexture != 0; }
    bool hasOcclusionTexture() const { return occlusionTexture != 0; }
    bool hasEmissiveTexture() const { return emissiveTexture != 0; }
};