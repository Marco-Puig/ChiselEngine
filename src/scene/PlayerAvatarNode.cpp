#include "scene/PlayerAvatarNode.h"
#include "rendering/Material.h"

#include <glad/glad.h>

#include <vector>

PlayerAvatarNode::PlayerAvatarNode(const std::string& name, const glm::vec3& color)
    : MeshNode(name) {
    build(color);
}

void PlayerAvatarNode::build(const glm::vec3& color) {
    const float width = 0.45f;
    const float height = 1.8f;
    const float depth = 0.45f;

    const glm::vec3 min(
        -width * 0.5f,
        0.0f,
        -depth * 0.5f
    );

    const glm::vec3 max(
        width * 0.5f,
        height,
        depth * 0.5f
    );

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    auto addQuad = [&](
        const glm::vec3& normal,
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d
    ) {
        const unsigned int base =
            static_cast<unsigned int>(vertices.size() / 8);

        const glm::vec3 positions[4] = {a, b, c, d};

        const glm::vec2 uvs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        for (int i = 0; i < 4; ++i) {
            vertices.push_back(positions[i].x);
            vertices.push_back(positions[i].y);
            vertices.push_back(positions[i].z);

            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            vertices.push_back(uvs[i].x);
            vertices.push_back(uvs[i].y);
        }

        indices.insert(
            indices.end(),
            {
                base,
                base + 1,
                base + 2,
                base,
                base + 2,
                base + 3
            }
        );
    };

    // +X
    addQuad(
        {1.0f, 0.0f, 0.0f},
        {max.x, min.y, max.z},
        {max.x, min.y, min.z},
        {max.x, max.y, min.z},
        {max.x, max.y, max.z}
    );

    // -X
    addQuad(
        {-1.0f, 0.0f, 0.0f},
        {min.x, min.y, min.z},
        {min.x, min.y, max.z},
        {min.x, max.y, max.z},
        {min.x, max.y, min.z}
    );

    // +Y
    addQuad(
        {0.0f, 1.0f, 0.0f},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z},
        {max.x, max.y, min.z},
        {min.x, max.y, min.z}
    );

    // -Y
    addQuad(
        {0.0f, -1.0f, 0.0f},
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {max.x, min.y, max.z},
        {min.x, min.y, max.z}
    );

    // +Z
    addQuad(
        {0.0f, 0.0f, 1.0f},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {max.x, max.y, max.z},
        {min.x, max.y, max.z}
    );

    // -Z
    addQuad(
        {0.0f, 0.0f, -1.0f},
        {max.x, min.y, min.z},
        {min.x, min.y, min.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z}
    );

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indices.size() * sizeof(unsigned int),
        indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(3 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        8 * sizeof(float),
        reinterpret_cast<void*>(6 * sizeof(float))
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    setMesh(
        vao,
        vbo,
        ebo,
        static_cast<int>(indices.size()),
        true
    );

    setBounds(min, max);

    std::vector<glm::vec3> collisionVertices = {
        {min.x, min.y, min.z},
        {max.x, min.y, min.z},
        {min.x, max.y, min.z},
        {max.x, max.y, min.z},
        {min.x, min.y, max.z},
        {max.x, min.y, max.z},
        {min.x, max.y, max.z},
        {max.x, max.y, max.z}
    };

    setCollisionVertices(std::move(collisionVertices));

    Material material;
    material.baseColorFactor = glm::vec4(color, 1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.65f;

    setMaterial(material);
}