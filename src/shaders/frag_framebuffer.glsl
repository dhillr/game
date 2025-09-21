#version 330 core

uniform sampler2D colorbuf;

in vec2 texCoord;
out vec4 FragColor;

void main() {
    FragColor = texture(colorbuf, texCoord);
}