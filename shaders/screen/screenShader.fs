#version 430 core

in vec2 texCoords;
out vec4 colour;

uniform sampler2D screenSampler;
uniform sampler2D depthSampler;

void main() {
  colour = texture(screenSampler, texCoords);
}
