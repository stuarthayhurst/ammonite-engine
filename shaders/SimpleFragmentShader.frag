#version 330 core

//Interpolated values from the vertex shaders
in vec3 fragmentColour;

//Output final colour
out vec3 color;

void main() {
  //Specified by the vertex shader
  color = fragmentColour;
}
