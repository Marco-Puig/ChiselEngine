#version 450 core
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_norm;
layout (location = 2) in vec2 in_texCoords; // Input texture coordinates

out vec2 TexCoords; // Pass texture coordinates to fragment shader

layout(std140) uniform TransformBuffer {
    mat4 world;
    mat4 viewproj;
};

void main() {
    TexCoords = in_texCoords;
    gl_Position = viewproj * world * vec4(in_pos, 1.0);
}
