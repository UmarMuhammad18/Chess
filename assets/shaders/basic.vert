#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec4 uNdc; // x, y, w, h in NDC (origin bottom-left)

void main() {
    vec2 uv = aPos.xy * 0.5 + 0.5;
    vec2 pos = uNdc.xy + uv * uNdc.zw;
    gl_Position = vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
