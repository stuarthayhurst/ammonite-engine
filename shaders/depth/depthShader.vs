#version 430 core

layout(location = 0) in vec3 inPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

void main() {
  //Output position, in light space
  gl_Position = lightSpaceMatrix * modelMatrix * vec4(inPosition, 1);
}
