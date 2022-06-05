#version 430 core

layout(location = 0) in vec3 inPosition;
uniform mat4 MVP;

void main() {
  //Output position of the vertex
  gl_Position = MVP * vec4(inPosition, 1);
}
