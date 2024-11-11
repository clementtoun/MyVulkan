#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBiTangent;
layout(location = 4) in vec2 inTexCoord;
layout(location = 5) in vec3 inColor;

layout(location = 0) out vec3 Normal;
layout(location = 1) out vec2 TexCoord;
layout(location = 2) out vec3 FragColor;
layout(location = 3) out vec3 WorldFragPos;
layout(location = 4) out mat3 ModelToTangentLocal;

layout (set=1, binding=0) uniform Models
{
    mat4 model;
};

layout (set=0, binding=0) uniform Scene
{
    mat4 view;
    mat4 projection;
    vec3 camPosition;
    int padding;
    int numDirectionalLights;
    int numPointLights;
};

void main() {
    vec4 pos = model * vec4(inPosition, 1.0);
    WorldFragPos = pos.xyz / pos.w;

    gl_Position = projection * view * pos;

    mat3 orthoModelMV = mat3(transpose(inverse(model)));
    Normal = normalize(orthoModelMV * inNormal);
    TexCoord = inTexCoord;

    vec3 T = normalize(orthoModelMV * inTangent);
    vec3 B = normalize(orthoModelMV * inBiTangent);
    vec3 N = Normal;

    ModelToTangentLocal = mat3(T, B, N);

    FragColor = inColor;
}