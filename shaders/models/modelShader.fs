#version 430 core

//Data structure to handle input from shader storage buffer object
struct LightSource {
  vec3 geometry;
  vec3 diffuse;
  vec3 specular;
  vec3 power;
};

//Lighting inputs from shader storage buffer
layout (std430, binding = 0) readonly buffer LightPropertiesBuffer {
  LightSource lightSources[];
};

//Input fragment data, from vertex shader
in FragmentDataOut {
  vec3 fragPos;
  vec3 normal;
  vec2 texCoord;
} fragData;

//Ouput data
out vec3 outputColour;

//Engine inputs
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform samplerCubeArrayShadow shadowCubeMap;
uniform vec3 ambientLight;
uniform vec3 cameraPos;
uniform float shadowFarPlane;
uniform int lightCount;

float calcShadow(int layer, vec3 fragPos, vec3 lightPos) {
  //Get depth of current fragment
  vec3 lightToFrag = fragPos - lightPos;
  float currentDepth = length(lightToFrag) / shadowFarPlane;

  float bias = 0.01f;
  return 1.0f - texture(shadowCubeMap, vec4(lightToFrag, layer), currentDepth - bias).r;
}

vec3 calcLight(LightSource lightSource, vec3 normal, vec3 fragPos, vec3 lightDir) {
  //Diffuse component
  float diff = clamp(dot(lightDir, normal), 0.0, 1.0);
  vec3 diffuse = diff * lightSource.diffuse;

  //Specular component
  vec3 specular = vec3(0.0f);
  if (diff > 0.0f) {
    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(dot(normal, halfwayDir), 2.0);
    specular = lightSource.specular * spec;
    specular *= texture(specularSampler, fragData.texCoord).rgb;
  }

  //Attenuation of the source
  float dist = distance(lightSource.geometry, fragPos);
  float attenuation = lightSource.power.x / (dist * dist);

  return (diffuse + specular) * attenuation;
}

void main() {
  //Base colour of the fragment
  vec3 materialColour = texture(diffuseSampler, fragData.texCoord).rgb;
  vec3 lightColour = vec3(0.0f);

  //Calculate lighting influence from each light source
  for (int i = 0; i < lightCount; i++) {
    vec3 lightDir = normalize(lightSources[i].geometry - fragData.fragPos);

    //Final contribution from the current light source
    float shadow = calcShadow(i, fragData.fragPos, lightSources[i].geometry);
    vec3 light = calcLight(lightSources[i], fragData.normal, fragData.fragPos, lightDir);
    lightColour += (1.0f - shadow) * light;
  }

  //Final fragment colour, from ambient, diffuse, specular and shadow components
  outputColour = (ambientLight + lightColour) * materialColour;
}
