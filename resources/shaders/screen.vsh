#version 460

out vec2 fragCoord;

const vec2 verts[6] = vec2[](
    vec2(-1,-1),
    vec2( 1,-1),
    vec2( 1, 1),
    vec2(-1,-1),
    vec2( 1, 1),
    vec2(-1, 1)
);

void main() {
  vec2 vPos = verts[gl_VertexID];
  gl_Position = vec4(vPos, 0, 1);
  fragCoord = vPos*0.5+0.5;
}
