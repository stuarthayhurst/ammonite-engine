#version 430 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

uniform mat4 shadowMatrices[6];
uniform int shadowMapIndex;

out vec4 fragPos;

void main() {
  //For every triangle vertex, output the position on the cubemap
  for (int face = 0; face < 6; face++) {
    gl_Layer = (shadowMapIndex * 6) + face;

    for (int i = 0; i < 3; i++) {
      fragPos = gl_in[i].gl_Position;
      gl_Position = shadowMatrices[face] * fragPos;

      EmitVertex();
    }

    EndPrimitive();
  }
}
