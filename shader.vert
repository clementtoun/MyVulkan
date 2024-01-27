#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragColor;

layout (set=0, binding=0) uniform Models
{
    mat4 model;
};

layout (set=1, binding=0) uniform VP
{
    mat4 view;
    mat4 projection;
};

void main() {
    vec4 pos = model * vec4(inPosition, 1.0);
    gl_Position = projection * view * pos;
    fragPos = pos.xyz / pos.w;

    Normal = normalize(mat3(transpose(inverse(model))) * inNormal);
    fragColor = inColor;
}