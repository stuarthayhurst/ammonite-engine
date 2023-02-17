#version 430 core

layout (location = 0) in vec2 inPosition;

uniform float progress;
uniform float width;
uniform float height;
uniform float heightOffset;

void main() {
  //Scale to width and fill by progress
  const float x = width * (((inPosition.x + 1) * progress) - 1);

  //Scale to height
  float y = (((inPosition.y + 1) * height) - 1);
  //Apply offset
  y = y + (2.0f * (1 - heightOffset));

  gl_Position = vec4(x, y, 0.0, 1.0);
}
