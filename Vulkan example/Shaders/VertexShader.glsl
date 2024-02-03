#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 normal;

layout(location = 0) out vec4 fragColor;
layout(location = 4) out vec4 worldPosition;
layout(location = 8) out vec4 modNormal;


layout(push_constant) uniform worldBuffer
{
  mat4x4 worldProjection;
  mat4x4 viewProjection;
  mat4x4 objectPosition;
  vec4 lightPos;
  float lightStrenght;
  float[3] pad;
};

void main() {

    mat4 tpInverse = mat4(transpose(mat3(inverse(objectPosition))));
    modNormal = normalize(tpInverse * normal);
    worldPosition =  objectPosition * vec4(inPosition, 1.0); 
    vec4 pos =  worldProjection * viewProjection * worldPosition;
    gl_Position = pos;
    fragColor = inColor;
}