#version 430 core

//Data structure to handle input from shader storage buffer object
struct LightSource {
  vec3 geometry;
  vec3 diffuse;
  vec3 specular;
  vec3 power;
};

//Lighting inputs from shader storage buffer
layout (std430, binding = 0) buffer LightPropertiesBuffer {
  LightSource lightSources[];
};

out vec3 colour;
uniform uint lightIndex;

void main() {
  //Use light source colour as fragment colour
  colour = lightSources[lightIndex].diffuse;
}
