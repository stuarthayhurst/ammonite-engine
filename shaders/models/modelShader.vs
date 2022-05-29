#version 430 core

//Input vertex data, changes every execution
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 2) in vec3 vertexNormal_modelspace;

//Output data, interpolated for each fragment
out vec2 texCoord;
out vec3 Position_worldspace;
out vec3 Normal_cameraspace;
out vec3 EyeDirection_cameraspace;

//Constants from C++
uniform mat4 MVP;
uniform mat4 V;
uniform mat4 M;
uniform mat3 normalMatrix;

void main() {
  //Output position of the vertex, in clip space (MVP * position)
  gl_Position = MVP * vec4(vertexPosition_modelspace, 1);

  //Position of the vertex, in worldspace (M * position)
  Position_worldspace = (M * vec4(vertexPosition_modelspace, 1)).xyz;

  //Vector from vertex to camera (camera space - camera at 0, 0, 0)
  vec3 vertexPosition_cameraspace = (V * M * vec4(vertexPosition_modelspace, 1)).xyz;
  EyeDirection_cameraspace = vec3(0, 0, 0) - vertexPosition_cameraspace;

  //Normal of the the vertex (camera space)
  Normal_cameraspace = (mat3(V) * normalMatrix * vertexNormal_modelspace).xyz;

  //Coordinate of the vertex
  texCoord = vertexTexCoord;
}
