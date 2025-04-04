#version 430 core

layout (location = 0) in vec3 inPosition;

out vec3 texCoords;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main() {
  texCoords = inPosition;
  gl_Position = (projectionMatrix * viewMatrix * vec4(inPosition, 1.0)).xyww;
}
