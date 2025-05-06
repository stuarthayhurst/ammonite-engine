#version 430 core

out vec4 colour;

uniform vec3 progressColour;

void main() {
  colour = vec4(progressColour, 1.0f);
}
