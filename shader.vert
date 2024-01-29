#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec3 FragColor;
layout(location = 2) out vec3 WorldFragPos;
layout(location = 3) out vec3 CamPosition;

layout (set=0, binding=0) uniform Models
{
    mat4 model;
};

layout (set=1, binding=0) uniform Camera
{
    mat4 view;
    mat4 projection;
    vec3 camPosition;
};

void main() {
    vec4 pos = model * vec4(inPosition, 1.0);
    WorldFragPos = pos.xyz / pos.w;

    CamPosition = camPosition;

    gl_Position = projection * view * pos;

    Normal = mat3(transpose(inverse(model))) * inNormal;
    FragColor = inColor;
}