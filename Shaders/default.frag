#version 430 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D uTexture;
uniform vec3 uLightColor;
uniform vec3 uLightDir;
uniform vec3 uViewPos;

void main() {
    // Sample texture
    vec4 texColor = texture(uTexture, TexCoord);
    
    // Ambient lighting
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * uLightColor;
    
    // Diffuse lighting (Lambertian)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-uLightDir);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor;
    
    // Combine and apply to texture
    vec3 result = (ambient + diffuse) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
}
