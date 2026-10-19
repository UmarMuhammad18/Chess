#version 330 core
// Unit quad (0..1) placed at a pixel-space rectangle; origin is the top-left of the window
layout (location = 0) in vec2 aPos;

uniform vec2 uScreen;   // framebuffer size in pixels
uniform vec4 uRect;     // x, y, width, height in pixels
uniform vec4 uUV;       // u0, v0, u1, v1 (texture space, v down)

out vec2 vLocal;
out vec2 vUV;

void main() {
    vec2 px = uRect.xy + aPos * uRect.zw;
    gl_Position = vec4(px.x / uScreen.x * 2.0 - 1.0, 1.0 - px.y / uScreen.y * 2.0, 0.0, 1.0);
    vLocal = aPos;
    vUV = mix(uUV.xy, uUV.zw, aPos);
}
