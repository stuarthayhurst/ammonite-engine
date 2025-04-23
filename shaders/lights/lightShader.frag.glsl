#version 430 core

//Data structure to handle input from shader storage buffer object
//specular.w is actually power
struct LightSource {
  vec3 geometry;
  vec3 diffuse;
  vec4 specular;
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
