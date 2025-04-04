#version 430 core

in vec3 texCoords;
out vec4 colour;

uniform samplerCube skyboxSampler;

void main() {
  colour = texture(skyboxSampler, texCoords);
}
