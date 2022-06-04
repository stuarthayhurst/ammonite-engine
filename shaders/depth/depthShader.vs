#version 430 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

void main() {
  //Position of vertex, in light space
  gl_Position = lightSpaceMatrix * modelMatrix * vec4(vertexPosition, 1);
}
