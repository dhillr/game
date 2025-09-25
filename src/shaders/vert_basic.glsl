#version 330 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 st;

uniform vec2 camera_offset;
uniform vec2 offset;
uniform float rotation_amount;

float aspect_ratio = 16 / 9;

mat2 rotation_matrix = mat2(
    cos(rotation_amount), -sin(rotation_amount),
    sin(rotation_amount), cos(rotation_amount)
);

vec2 rotate(vec2 v, float theta) {
    return vec2(v.x * cos(theta) - v.y * sin(theta), v.y * cos(theta) + v.x * sin(theta));
}

out vec2 texCoord;

void main() {
    gl_Position = vec4(rotate(pos, rotation_amount) - camera_offset + offset, 0, 1);
    texCoord = st;
}