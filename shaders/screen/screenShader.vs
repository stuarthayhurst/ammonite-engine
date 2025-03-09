#version 430 core

layout (location = 0) in vec2 inPosition;

out vec2 texCoords;

void main() {
  texCoords = (inPosition + 1.0) / 2.0;
  gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
}
