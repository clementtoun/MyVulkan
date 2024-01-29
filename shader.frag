#version 450

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec3 FragColor;
layout(location = 2) in vec3 WorldFragPos;
layout(location = 3) in vec3 CamPosition;

layout(location = 0) out vec4 outColor;

const float PI = 3.1415926535897932384626433832795;
const float MIN_FLT = 1.175494351e-32;

float D(float NH, float alpha)
{
    float alphaSq = alpha*alpha;
    float d = NH*NH * (alphaSq - 1.) + 1.;
    return alphaSq / (PI * d*d);
}

float G1(float X, float k)
{
    return X / (X * (1. - k) + k);
}

float GGX(float NL, float NV, float Roughness)
{
    float k = (Roughness + 1.);
    k = k*k / 8.;
    return G1(NL, k) * G1(NV, k); 
}

vec3 FUnreal(float VH, vec3 F0)
{
    return F0 + (1. - F0) * pow(2, (-5.55473 * VH - 6.98316) * VH);
}

void main() {
    vec3 LigthColor = vec3(1.);

    vec3 L = normalize(vec3(1., 1., -0.1));
    vec3 N = normalize(Normal);
    vec3 V = normalize(CamPosition - WorldFragPos);
    vec3 H = normalize(V + L);

    float NH = max(0., dot(N, H));
    float NL = max(0., dot(N, L));
    float NV = max(0., dot(N, V));
    float VH = max(0., dot(V, H));

    float Metallic = 0.5;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, FragColor, Metallic);

    float Roughness = 0.25;
    float alpha = Roughness*Roughness;

    vec3 F = FUnreal(VH, F0);
    vec3 kD = (1. - F) * (1. - Metallic);

    vec3 fDiff = FragColor / PI;
    vec3 fSpec = D(NH, alpha) * F * GGX(NL, NV, Roughness) / (4. * NL * NV + MIN_FLT);

    vec3 ambiant = F0 * 0.04;

    vec3 color = ambiant + PI * (kD * fDiff + fSpec) * LigthColor * NL;

    color = color / (color + vec3(1.));
    color = pow(color, vec3(1./2.2)); 

    outColor = vec4(color, 1.);
}