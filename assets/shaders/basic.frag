#version 330 core
in vec2 vLocal;
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;
// 0 = solid colour, 1 = texture, 2 = green-screen keyed texture, 3 = filled disc, 4 = ring
uniform int uMode;

void main() {
    if (uMode == 1) {
        FragColor = texture(uTexture, vUV) * uColor;
    } else if (uMode == 2) {
        // The piece atlas has a chroma-green background and no alpha channel:
        // derive alpha from how much green dominates, then remove green spill on edges
        vec4 c = texture(uTexture, vUV);
        float green = c.g - max(c.r, c.b);
        float alpha = 1.0 - smoothstep(0.12, 0.30, green);
        if (alpha < 0.02) discard;
        c.g = min(c.g, max(c.r, c.b) + 0.04);
        FragColor = vec4(c.rgb, alpha) * uColor;
    } else if (uMode == 3 || uMode == 4) {
        float d = length(vLocal - 0.5) * 2.0;   // 0 at centre, 1 at the edge
        float aa = fwidth(d) * 1.5;
        float a = 1.0 - smoothstep(1.0 - aa, 1.0, d);
        if (uMode == 4) a *= smoothstep(0.80 - aa, 0.80, d);
        FragColor = vec4(uColor.rgb, uColor.a * a);
    } else {
        FragColor = uColor;
    }
}
