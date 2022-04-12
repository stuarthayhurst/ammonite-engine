#version 330 core

//Input data from vertex shader
in vec2 texCoord;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec3 LightDirection_cameraspace;

//Ouput data
out vec3 colour;

//Constants from C++
uniform sampler2D textureSampler;
uniform vec3 LightPosition_worldspace;

//Constants
uniform vec3 LightColour = vec3(1, 1, 1);
uniform float LightPower = 50.0f;

void main() {
  //Material properties
  vec3 MaterialDiffuseColour = texture(textureSampler, texCoord).rgb;
  vec3 MaterialAmbientColour = vec3(0.1, 0.1, 0.1) * MaterialDiffuseColour;
  vec3 MaterialSpecularColour = vec3(0.3, 0.3, 0.3);

  //Distance to the light
  float distance = length(LightPosition_worldspace - Position_worldspace);

  //Normal of the computed fragment (camera space)
  vec3 normal = normalize(Normal_cameraspace);
  //Direction of the light (from the fragment to the light)
  vec3 lightDirection = normalize(LightDirection_cameraspace);
  //Cosine of the angle between the normal and the light direction
  float cosTheta = clamp(dot(normal, lightDirection), 0, 1);

  //Eye vector (towards the camera)
  vec3 eyeDirection = normalize(EyeDirection_cameraspace);
  //Direction in which the triangle reflects the light
  vec3 reflectionDirection = reflect(-lightDirection, normal);
  // Cosine of the angle between the Eye vector and the Reflect vector,
  float cosAlpha = clamp(dot(eyeDirection, reflectionDirection), 0, 1);

  //Calculate diffuse and specular components
  MaterialDiffuseColour = MaterialDiffuseColour * LightColour * LightPower * cosTheta / (distance * distance);
  MaterialSpecularColour = MaterialSpecularColour * LightColour * LightPower * pow(cosAlpha, 5) / (distance * distance);

  //Calculate final colour
  colour = MaterialAmbientColour + MaterialDiffuseColour + MaterialSpecularColour;
}
