#version 330 core

uniform vec3 color;
out vec4 col;

void main() {
    col = vec4(color, 1);
}