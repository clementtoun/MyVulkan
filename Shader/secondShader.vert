#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBiTangent;
layout(location = 4) in vec2 inTexCoord;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 CamPosition;
layout(location = 1) out flat int NumDirectionalLights;
layout(location = 2) out flat int NumPointLights;

layout (set=0, binding=0) uniform Scene
{
    mat4 view;
    mat4 projection;
    vec3 camPosition;
    int padding1;
    float time;
    int numDirectionalLights;
    int numPointLights;
};

void main() {
    CamPosition = vec3(camPosition);
    NumDirectionalLights = numDirectionalLights;
    NumPointLights = numPointLights;

    gl_Position = vec4(inPosition, 1.);
}