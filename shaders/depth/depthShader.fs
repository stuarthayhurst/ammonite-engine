#version 430 core

in vec4 fragPos;

uniform vec3 lightPos;
uniform float farPlane;

void main() {
  float lightDistance = distance(fragPos.xyz, lightPos);

  //Save distance, mapped distance to [0, 1]
  gl_FragDepth = lightDistance / farPlane;
}
