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
layout(std430, binding = 0) buffer LightPropertiesBuffer {
  RawLightSource lightSources[];
};

//Input fragment data, from vertex shader
in FragmentDataOut {
  vec3 fragPos;
  vec4 fragPos_lightspace;
  vec3 normal;
  vec2 texCoord;
} fragData;

//Ouput data
out vec3 outputColour;

//Engine inputs
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;
uniform mat4 V;
uniform vec3 ambientLight;
uniform vec3 cameraPos;

float calcShadow(vec4 fragPos_lightspace, vec3 normal, vec3 lightDir) {
  //Perspective divide (range [-1, 1])
  vec3 projCoords = fragPos_lightspace.xyz / fragPos_lightspace.w;

  //Shift to within [0, 1]
  projCoords = projCoords * 0.5 + 0.5;

  //Get depth of current fragment from light's perspective
  float currentDepth = projCoords.z;

  //Check whether fragment is in shadow
  float shadow = 0.0;
  float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
  vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
      if (currentDepth - bias > pcfDepth) {
        shadow += 1.0;
      }
    }
  }
  shadow /= 9.0;

  //No shadow outside of calculated area
  if (projCoords.z > 1.0) {
    shadow = 0.0;
  }

  return shadow;
}

vec3 calcLight(LightSource lightSource, vec3 normal, vec3 fragPos, vec3 lightDir) {
    //Diffuse component
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = lightSource.diffuse * diff * lightSource.colour;

    //Specular component
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 2.0);
    vec3 specular = lightSource.specular * spec * lightSource.colour;

    //Attenuation of the source
    float dist = distance(lightSource.geometry, fragPos);
    float attenuation = lightSource.power / (dist * dist);

    return (diffuse + specular) * attenuation;
}

void main() {
  //Base colour of the fragment
  vec3 materialColour = texture(textureSampler, fragData.texCoord).rgb;
  vec3 lightColour = vec3(0.0f, 0.0f, 0.0f);
  float shadow = 0.0f;

  //Calculate lighting influence from each light source
  LightSource lightSource;
  int lightCount = lightSources.length();
  for (int i = 0; i < lightCount; i++) {
    lightSource.geometry = lightSources[i].geometry.xyz;
    lightSource.colour = lightSources[i].colour.xyz;
    lightSource.diffuse = lightSources[i].diffuse.xyz;
    lightSource.specular = lightSources[i].specular.xyz;
    lightSource.power = lightSources[i].power.x;

    vec3 lightDir = normalize(lightSource.geometry - fragData.fragPos);

    //Final contribution from the current light source
    shadow = calcShadow(fragData.fragPos_lightspace, fragData.normal, lightDir);
    lightColour += calcLight(lightSource, fragData.normal, fragData.fragPos, lightDir);
  }

  //Final fragment colour, from ambient, diffuse, specular and shadow components
  outputColour = (ambientLight + ((1.0 - shadow) * lightColour)) * materialColour;
}
