#version 450

layout(location = 0) in vec3 CamPosition;
layout(location = 1) in vec3 WorldDirection;


layout (set=0, binding=1) uniform samplerCube cubeMapTexture;

layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputFragPos;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputColor;
layout (input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputPbr;
layout (input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput inputEmissive;

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

    vec4 Nw = subpassLoad(inputNormal);
    vec4 BaseColor = subpassLoad(inputColor);
    vec4 Emissive = subpassLoad(inputEmissive);

    if (Nw.w == 1.)
    {
        outColor = texture(cubeMapTexture, WorldDirection);
    }
    else
    {
        vec3 FragPos = subpassLoad(inputFragPos).xyz;

        vec3 LigthColor = vec3(1.);

        vec3 L0 = normalize(vec3(4., 15., -10.) - FragPos);
        vec3 L1 = normalize(vec3(.5, 1., 0.5));

        vec3 La[2] = {L0, L1};

        vec3 N = Nw.xyz;
        vec3 V = normalize(CamPosition - FragPos);

        float opacity = BaseColor.a;
        vec3 albedo = BaseColor.rgb;

        vec3 MetallicRoughnessAO = subpassLoad(inputPbr).rgb;

        float AO = MetallicRoughnessAO.b;

        float Metallic = MetallicRoughnessAO.g;
        vec3 F0 = mix(vec3(0.04), albedo, Metallic);

        float Roughness = MetallicRoughnessAO.r;
        float alpha = Roughness*Roughness;

        vec3 C_Diff = albedo * (1. - Metallic);
        vec3 fDiff = (1. / PI) * C_Diff;

        vec3 color = vec3(0.);

        for(int i = 0; i < 2; i++)
        {
            vec3 L = La[i];
            vec3 H = normalize(V + L);

            float NH = max(0., dot(N, H));
            float NL = max(0., dot(N, L));
            float NV = max(0., dot(N, V));
            float VH = max(0., dot(V, H));

            vec3 F = FUnreal(VH, F0);

            vec3 fSpec = vec3(D(NH, alpha) * GGX(NL, NV, Roughness) / (4. * NL * NV + MIN_FLT));
            vec3 BRDF = mix(fDiff, fSpec, F);

            color += BRDF * LigthColor * NL;
        }

        vec3 ambiant = vec3(0.004) * albedo * AO;

        color += ambiant + Emissive.rgb;

        color = color / (color + 1.);

        outColor = vec4(color, 1.);
    }
}