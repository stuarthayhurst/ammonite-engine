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

in vec4 fragPos;

uniform uint shadowMapIndex;
uniform float shadowFarPlane;

void main() {
  float lightDistance = distance(fragPos.xyz, lightSources[shadowMapIndex].geometry);

  //Save distance, mapped distance to [0, 1]
  gl_FragDepth = lightDistance / shadowFarPlane;
}
