#version 330 core

//Input vertex data, different for all executions of this shader
layout(location = 0) in vec3 vertexPosition_modelspace;
//Input colour data, 1 corresponds to 1 in glVertexAttribPointer
layout(location = 1) in vec3 vertexColour;

//Constants
uniform mat4 MVP;

//Output data, will be interpolated for each fragment
out vec3 fragmentColour;

void main() {
  //Output position of the vertex, in clip space : MVP * position
  gl_Position =  MVP * vec4(vertexPosition_modelspace,1);

  //The colour of each vertex will be interpolated to produce the colour of each fragment
  fragmentColour = vertexColour;
}
