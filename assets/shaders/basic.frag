#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform vec4 uvOffset;
uniform vec4 uTint;
uniform float uUseTex;

void main() {
    vec2 final_uv = vec2(TexCoord.x * uvOffset.z + uvOffset.x,
                         TexCoord.y * uvOffset.w + uvOffset.y);
    vec4 texColor = texture(ourTexture, final_uv);
    vec4 color = mix(vec4(1.0), texColor, uUseTex) * uTint;
    if (color.a < 0.05) discard;
    FragColor = color;
}
