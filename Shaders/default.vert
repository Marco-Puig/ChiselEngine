#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uModel;
uniform mat4 uViewProj;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    
    // Calculate normal matrix to handle non-uniform scaling
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    Normal = normalize(normalMatrix * aNormal);
    
    TexCoord = aTexCoord;
    gl_Position = uViewProj * vec4(FragPos, 1.0);
}
