#version 430 core

//Input vertex data
layout(location = 0) in vec3 vertexPosition_modelspace;

//Engine inputs
uniform mat4 MVP;

void main() {
  //Output position of the vertex, in clip space (MVP * position)
  gl_Position = MVP * vec4(vertexPosition_modelspace, 1);
}
