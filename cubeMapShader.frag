#version 450

layout(location = 0) in vec3 WorldFragPos;

layout (set=0, binding=1) uniform samplerCube cubeMapTexture;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec4 outPbr;
layout(location = 4) out vec4 outEmissive;

void main()
{
    outPosition = vec4(0.);
    outNormal = vec4(0., 0., 0., 1.); 
    outColor = texture(cubeMapTexture, WorldFragPos);
    outPbr = vec4(0.);
    outEmissive = vec4(0.);
}