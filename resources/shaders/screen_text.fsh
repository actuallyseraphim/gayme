#version 460

in vec2 fragCoord;
out vec3 fragColor;

uniform usampler2D screenTexture;

layout(std430, binding = 0) buffer DataBlock {
    uint font[];
};

void main() {
  uint glyph = texture(screenTexture, fragCoord).r;
  uint px = uint(gl_FragCoord.x)%8;
  uint py = 7-uint(gl_FragCoord.y)%8;
  uint p1 = font[2*glyph+py/4];
  py = py % 4;
  bool pixel = (1 == (p1 >> (px+(py<<3))&1));
  fragColor = vec3(pixel);
}
