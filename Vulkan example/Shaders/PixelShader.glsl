#version 450
layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColors;
void main() {
    outColors = fragColor;
}   