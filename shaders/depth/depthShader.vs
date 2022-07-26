#version 430 core

layout (location = 0) in vec3 inPosition;

uniform mat4 modelMatrix;

void main() {
  //Output position, in model space
  gl_Position = modelMatrix * vec4(inPosition, 1);
}
