#version 430 core

//Input data from vertex shader
in vec2 texCoord;
in vec3 Position_worldspace;
in vec3 Normal_cameraspace;
in vec3 EyeDirection_cameraspace;
in vec4 FragPos_lightspace;

//Ouput data
out vec3 colour;

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

//Engine inputs
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;
uniform mat4 V;
uniform vec3 ambientLight;

float calcShadow(vec4 FragPos_lightspace) {
  //Perspective divide (Range [-1, 1])
  vec3 projCoords = FragPos_lightspace.xyz / FragPos_lightspace.w;

  //Shift to within [0, 1]
  projCoords = projCoords * 0.5 + 0.5;

  //Get depth of current fragment from light's perspective
  float currentDepth = projCoords.z;

  //Check whether fragment is in shadow
  float shadow = 0.0;
  float bias = 0.0005;
  vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
      shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
      if (currentDepth - bias > pcfDepth) {
        shadow += 1.0;
      }
    }
  }

  //shadow /= 9.0;

  return shadow / 9.0;
}

vec3 calcDiffuseLight(LightSource lightSource, vec3 materialColour, vec3 lightDirection, vec3 normal) {
  //Cosine of the angle between the normal and light direction
  float cosTheta = clamp(dot(normal, lightDirection), 0, 1);

  return(lightSource.diffuse * materialColour * lightSource.colour * lightSource.power * cosTheta);
}

vec3 calcSpecularLight(LightSource lightSource, vec3 lightDirection, vec3 normal, vec3 eyeDirection) {
  //Only keep direction component
  eyeDirection = normalize(eyeDirection);

  //Direction of refledted light
  vec3 reflectionDirection = reflect(-lightDirection, normal);
  //Cosine of the angle between the eye and reflection vectors
  float cosAlpha = clamp(dot(eyeDirection, reflectionDirection), 0, 1);

  return(lightSource.specular * lightSource.colour * lightSource.power * pow(cosAlpha, 5));
}

vec3 calcPointLight(LightSource lightSource, vec3 materialColour, vec3 fragPos, vec3 normal, vec3 eyeDirection) {
  //Vector from vertex to light (camera space)
  vec3 LightPosition_cameraspace = (V * vec4(lightSource.geometry, 1)).xyz;
  vec3 LightDirection_cameraspace = LightPosition_cameraspace + eyeDirection;

  //Distance to the light
  float lightDistanceSqr = pow(distance(lightSource.geometry, fragPos), 2);

  //Direction of the light (from the fragment to the light)
  vec3 lightDirection = normalize(LightDirection_cameraspace);

  //Calculate lighting components
  vec3 diffuse = calcDiffuseLight(lightSource, materialColour, lightDirection, normal);
  vec3 specular = calcSpecularLight(lightSource, lightDirection, normal, eyeDirection);

  return((diffuse + specular) / lightDistanceSqr);
}

vec3 calcDirectionalLight(LightSource lightSource, vec3 materialColour, vec3 normal, vec3 eyeDirection) {
  //Direction of the light (from the light to the fragment)
  vec3 lightDirection = normalize(-(V * vec4(lightSource.geometry, 0)).xyz);

  //Calculate lighting components
  vec3 diffuse = calcDiffuseLight(lightSource, materialColour, lightDirection, normal);
  vec3 specular = calcSpecularLight(lightSource, lightDirection, normal, eyeDirection);

  return(diffuse + specular);
}

void main() {
  //Base colour of the fragment
  vec3 materialColour = texture(textureSampler, texCoord).rgb;
  colour = vec3(0.0, 0.0, 0.0);
  vec3 lightComp = vec3(0.0, 0.0, 0.0);

  //Normal of the fragment (camera space)
  vec3 normal = normalize(Normal_cameraspace);

  //Eye vector (towards the camera)
  vec3 eyeDirection = EyeDirection_cameraspace;

  //Calculate lighting influence from each light source
  LightSource lightSource;
  int lightCount = lightSources.length();
  for (int i = 0; i < lightCount; i++) {
    lightSource.geometry = lightSources[i].geometry.xyz;
    lightSource.colour = lightSources[i].colour.xyz;
    lightSource.diffuse = lightSources[i].diffuse.xyz;
    lightSource.specular = lightSources[i].specular.xyz;
    lightSource.power = lightSources[i].power.x;

    //Calculate fragment colour from current light
    lightComp += calcPointLight(lightSource, materialColour, Position_worldspace, normal, eyeDirection);
  }

  float shadow = calcShadow(FragPos_lightspace);
  colour = (ambientLight + (1.0 - shadow) * lightComp) * materialColour;
}
