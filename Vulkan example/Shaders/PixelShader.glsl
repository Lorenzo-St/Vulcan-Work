#version 450
layout(location = 0) in vec4 fragColor;
layout(location = 4) in vec4 worldPosition;
layout(location = 8) in vec4 modNormal;


layout(location = 0) out vec4 outColors;

layout(push_constant) uniform lightInfo
{
  mat4x4 worldProjection;
  mat4x4 viewProjection;
  mat4x4 objectPosition;
  vec4 lightPos;
  float lightStrenght;
  float light_factor;
  float ambient_factor;
  float[1] pad;
};



void main() {

    // 2 Subtractions 
    vec3 vector = worldPosition.xyz - lightPos.xyz;
    vec4 normalized = vec4(normalize(vector), 0);
    // this is a.x * b.x + a.y * b.y, so 2 multis and 1 addition
    float magsquared = dot(vector, vector);
    // A multi and a division
    float spotfactor = min((lightStrenght*lightStrenght*lightStrenght)/magsquared, 1);

    float scalar = ambient_factor + light_factor * spotfactor;
    float d = dot(normalized, modNormal);
    if(d < 0)
    // 1 multi
        outColors = fragColor * scalar;
    else
        outColors = vec4(0,0,0,1);
}   