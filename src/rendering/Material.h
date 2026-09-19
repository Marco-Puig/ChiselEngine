#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

struct Material {
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    GLuint baseColorTexture = 0;
    GLuint normalTexture = 0;
    GLuint metallicRoughnessTexture = 0;
    GLuint emissiveTexture = 0;

    bool hasBaseColorTexture() const { return baseColorTexture != 0; }
    bool hasNormalTexture() const { return normalTexture != 0; }
    bool hasMetallicRoughnessTexture() const { return metallicRoughnessTexture != 0; }
    bool hasEmissiveTexture() const { return emissiveTexture != 0; }
};
