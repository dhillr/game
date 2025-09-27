#version 330 core

uniform bool is_enemy;

out vec4 col;

void main() {
    if (is_enemy) {
        col = vec4(1, 0, 0, 1);
    } else {
        col = vec4(1);
    }
}