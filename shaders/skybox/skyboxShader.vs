#version 430 core

layout (location = 0) in vec3 inPosition;

out vec3 texCoords;
uniform mat4 V;
uniform mat4 P;

void main() {
  texCoords = inPosition;
  gl_Position = (P * V * vec4(inPosition, 1.0)).xyww;
}
