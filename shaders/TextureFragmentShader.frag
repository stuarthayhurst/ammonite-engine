#version 330 core

//Interpolated values from the vertex shaders
in vec2 UV;

//Output final colour
out vec3 color;

//Constants
uniform sampler2D textureSampler;

void main() {
  //Output color of the texture at the specified UV
  color = texture(textureSampler, UV).rgb;
}
