#version 430 core

//Data structure to match input from shader storage buffer object
struct RawLightSource {
  vec4 geometry;
  vec4 colour;
  vec4 diffuse;
  vec4 specular;
  vec4 power;
};

//Cleaned up data structure
struct LightSource {
  vec3 geometry;
  vec3 colour;
  vec3 diffuse;
  vec3 specular;
  float power;
};

//Lighting inputs from shader storage buffer
layout (std430, binding = 0) buffer LightPropertiesBuffer {
  RawLightSource lightSources[];
};

out vec3 colour;
uniform int lightIndex;

void main() {
  //Use light source colour as fragment colour
  colour = lightSources[lightIndex].colour.xyz;
}
