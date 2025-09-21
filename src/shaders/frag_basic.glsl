#version 330 core

uniform sampler2D tex0;
uniform vec4 offset_mod;

in vec2 texCoord;
out vec4 FragColor;

void main() {
    FragColor = texture(tex0, mod(texCoord, offset_mod.zw) + offset_mod.xy);
    if (FragColor.a < 0.1) discard;
}