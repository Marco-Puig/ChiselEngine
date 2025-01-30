#version 450 core
in vec2 TexCoords; 
out vec4 fragColor;

uniform sampler2D texture_diffuse; 

void main() {
	fragColor = texture(texture_diffuse, TexCoords);
}