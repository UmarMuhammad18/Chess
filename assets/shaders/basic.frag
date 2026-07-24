#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform vec4 uvOffset; // x, y, width, height

void main() {
    vec2 final_uv = vec2(TexCoord.x * uvOffset.z + uvOffset.x, TexCoord.y * uvOffset.w + uvOffset.y);
    vec4 texColor = texture(ourTexture, final_uv);
    if(texColor.a < 0.1) discard; // discard transparent pixels
    FragColor = texColor;
}
