#version 330 core

//Input data from vertex shader
in vec2 texCoord;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;

//Ouput data
out vec3 colour;

struct PointLight {
  vec3 position;
  vec3 colour;
  float power;

  vec3 diffuse;
  vec3 specular;
};

struct DirectionalLight {
  vec3 direction;
  vec3 colour;
  float power;

  vec3 diffuse;
  vec3 specular;
};

//Engine inputs
uniform sampler2D textureSampler;
uniform mat4 V;
uniform PointLight lightSource;
uniform vec3 ambientLight;

vec3 calcPointLight(PointLight lightSource, vec3 materialDiffuse, vec3 fragPos, vec3 normal, vec3 eyeDirection) {
  //Vector from vertex to light (camera space)
  vec3 LightPosition_cameraspace = (V * vec4(lightSource.position, 1)).xyz;
  vec3 LightDirection_cameraspace = LightPosition_cameraspace + eyeDirection;

  eyeDirection = normalize(eyeDirection);

  //Distance to the light
  float lightDistanceSqr = pow(distance(lightSource.position, fragPos), 2);

  //Direction of the light (from the fragment to the light)
  vec3 lightDirection = normalize(LightDirection_cameraspace);
  //Cosine of the angle between the normal and light direction
  float cosTheta = clamp(dot(normal, lightDirection), 0, 1);

  //Direction of refledted light
  vec3 reflectionDirection = reflect(-lightDirection, normal);
  //Cosine of the angle between the eye and reflection vectors
  float cosAlpha = clamp(dot(eyeDirection, reflectionDirection), 0, 1);

  //Calculate lighting components
  vec3 diffuse = lightSource.diffuse * materialDiffuse * lightSource.colour * lightSource.power * cosTheta / lightDistanceSqr;
  vec3 specular = lightSource.specular * lightSource.colour * lightSource.power * pow(cosAlpha, 5) / lightDistanceSqr;

  return(diffuse + specular);
}

vec3 calcDirectionalLight(DirectionalLight lightSource, vec3 materialDiffuse, vec3 fragPos, vec3 normal, vec3 eyeDirection) {
  eyeDirection = normalize(eyeDirection);

  //Direction of the light (from the light to the fragment)
  vec3 lightDirection = normalize(-(V * vec4(lightSource.direction, 0)).xyz);
  //Cosine of the angle between the normal and light direction
  float cosTheta = clamp(dot(normal, lightDirection), 0, 1);

  //Direction of refledted light
  vec3 reflectionDirection = reflect(-lightDirection, normal);
  //Cosine of the angle between the eye and reflection vectors
  float cosAlpha = clamp(dot(eyeDirection, reflectionDirection), 0, 1);

  //Calculate lighting components
  vec3 diffuse = lightSource.diffuse * materialDiffuse * lightSource.colour * lightSource.power * cosTheta;
  vec3 specular = lightSource.specular * lightSource.colour * lightSource.power * pow(cosAlpha, 5);

  return(diffuse + specular);
}

void main() {
  //Base colour of the fragment
  vec3 materialDiffuseColour = texture(textureSampler, texCoord).rgb;

  //Normal of the fragment (camera space)
  vec3 normal = normalize(Normal_cameraspace);

  //Eye vector (towards the camera)
  vec3 eyeDirection = EyeDirection_cameraspace;

  //Calculate fragment colour
  colour = calcPointLight(lightSource, materialDiffuseColour, Position_worldspace, normal, eyeDirection);

  //Apply ambient lighting
  colour += ambientLight * materialDiffuseColour;
}
