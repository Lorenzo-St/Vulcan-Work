#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform worldBuffer
{
  mat4x4 worldProjection;
  mat4x4 viewProjection;
  mat4x4 objectPosition;
};

void main() {
    vec4 pos =  worldProjection * viewProjection * objectPosition * vec4(inPosition, 1.0); 
    gl_Position = pos;
    fragColor = inColor;
}