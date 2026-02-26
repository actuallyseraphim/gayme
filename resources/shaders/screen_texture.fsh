#version 460

in vec2 fragCoord;
out vec3 fragColor;

uniform usampler2D screenTexture;

void main() {
  fragColor = texture(screenTexture, fragCoord).rgb;
}
