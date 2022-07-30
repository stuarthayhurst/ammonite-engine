#version 430 core

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inTexCoord;

out vec2 texCoords;

void main() {
  texCoords = inTexCoord;
  gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
}
