#version 330 core

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 st;

uniform vec2 camera_offset;

out vec2 texCoord;

void main() {
    gl_Position = vec4(pos - camera_offset, 0, 1);
    texCoord = st;
}