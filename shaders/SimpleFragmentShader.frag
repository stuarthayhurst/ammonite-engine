#version 330 core

//Interpolated values from the vertex shaders
in vec3 fragmentColor;

//Output final colour
out vec3 color;

void main() {
  //Specified by the vertex shader
  color = fragmentColor;
}
