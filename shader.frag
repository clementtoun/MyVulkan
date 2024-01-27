#version 450

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 ligthDir = vec3(0.25, 1., -0.25);

    outColor = vec4(fragColor * max(0., dot(Normal, ligthDir)), 1.0);
}