#version 450

layout(location = 0) in vec3 inPosition;

layout (set=0, binding=0) uniform Scene
{
    mat4 view;
    mat4 projection;
    vec3 camPosition;
    int padding;
    int numDirectionalLights;
    int numPointLights;
};

layout(location = 0) out vec3 WorldFragPos;

void main()
{
    WorldFragPos = inPosition;

    mat4 rotView = mat4(mat3(view)); // remove translation from the view matrix
    vec4 clipPos = projection * rotView * vec4(inPosition, 1.0);

    gl_Position = clipPos.xyww;
}