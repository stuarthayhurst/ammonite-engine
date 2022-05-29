#version 430 core

//Ouput data
out vec3 colour;

//Engine inputs
uniform int lightIndex;

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
layout(std430, binding = 0) buffer LightPropertiesBuffer {
  RawLightSource lightSources[];
};

void main() {
  //Use light source colour
  colour = lightSources[lightIndex].colour.xyz;
}
