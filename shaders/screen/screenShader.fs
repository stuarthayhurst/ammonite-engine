#version 430 core

in vec2 texCoords;
out vec4 colour;

uniform sampler2D screenSampler;
uniform sampler2D depthSampler;

uniform bool focalDepthEnabled;
uniform float focalDepth;
uniform float blurStrength;

const float nearPlane = 0.1f;
uniform float farPlane;

void main() {
  if (!focalDepthEnabled) {
    colour = vec4(texture(screenSampler, texCoords));
    return;
  }

  //Calculate the offset from the focal depth and strength
  float offset = 1.0 / 75.0;
  float depth = texture(depthSampler, texCoords).x;
  depth = 1 - nearPlane / (farPlane + nearPlane - depth * (farPlane - nearPlane));
  depth = abs(depth - focalDepth);
  offset *= blurStrength * depth;

  vec2 offsets[9] = {
    vec2(-offset, offset),
    vec2(0.0f, offset),
    vec2(offset, offset),
    vec2(-offset, 0.0f),
    vec2(0.0f, 0.0f),
    vec2(offset, 0.0f),
    vec2(-offset, -offset),
    vec2(0.0f, -offset),
    vec2(offset, -offset)
  };

  float blurKernel[9] = {
    1.0 / 16, 2.0 / 16, 1.0 / 16,
    2.0 / 16, 4.0 / 16, 2.0 / 16,
    1.0 / 16, 2.0 / 16, 1.0 / 16
  };

  vec3 sampleTex[9];
  for (int i = 0; i < 9; i++) {
    sampleTex[i] = vec3(texture(screenSampler, texCoords.st + offsets[i]));
  }

  vec3 col = vec3(0.0);
  for (int i = 0; i < 9; i++) {
    col += sampleTex[i] * blurKernel[i];
  }

  colour = vec4(col, 1.0);
}
