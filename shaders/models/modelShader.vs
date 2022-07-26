#version 430 core

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 vertexTexCoord;

//Output fragment data, sent to fragment shader
out FragmentDataOut {
  vec3 fragPos;
  vec3 normal;
  vec2 texCoord;
} fragData;

uniform mat4 MVP;
uniform mat4 modelMatrix;
uniform mat3 normalMatrix;

void main() {
  //Position of the vertex, in worldspace
  fragData.fragPos = (modelMatrix * vec4(inPosition, 1)).xyz;

  //Vertex normal
  fragData.normal = normalize(normalMatrix * inNormal);

  //Vertex texture coord
  fragData.texCoord = vertexTexCoord;

  //Output position of the vertex
  gl_Position = MVP * vec4(inPosition, 1);
}
