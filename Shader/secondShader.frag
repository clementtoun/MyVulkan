#version 450

layout(location = 0) in vec3 CamPosition;
layout(location = 1) in flat int NumDirectionalLights;
layout(location = 2) in flat int NumPointLights;

layout (input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inputFragPos;
layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputNormal;
layout (input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inputColor;
layout (input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inputPbr;
layout (input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput inputEmissive;

layout(location = 0) out vec4 outColor;

struct UniformDirectionalLight {
    vec3 Color;
    int padding;
    vec3 Direction;
    int padding1;
};

struct UniformPointLight {
    vec3 Color;
    int padding;
    vec3 Position;
    int padding1;
};

layout (std140, set=0, binding=2) readonly buffer DirectionalLightBuffer {
    UniformDirectionalLight lights[];
} directionalLightBuffer;

layout (std140, set=0, binding=3) readonly buffer PointLightBuffer {
    UniformPointLight lights[];
} pointLightBuffer;

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

    vec3 color = vec3(0.);

    if (Nw.w == 1.)
    {
        color = BaseColor.xyz;
    }
    else
    {
        vec3 FragPos = subpassLoad(inputFragPos).xyz;

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

        float NV = max(0., dot(N, V));

        for(int i = 0; i < NumDirectionalLights; i++)
        {
            UniformDirectionalLight directionalLight = directionalLightBuffer.lights[i];
            
            vec3 L = normalize(-directionalLight.Direction);
            vec3 H = normalize(V + L);

            float NH = max(0., dot(N, H));
            float NL = max(0., dot(N, L));
            float VH = max(0., dot(V, H));

            vec3 F = FUnreal(VH, F0);

            vec3 fSpec = vec3(D(NH, alpha) * GGX(NL, NV, Roughness) / (4. * NL * NV + MIN_FLT));
            vec3 BRDF = mix(fDiff, fSpec, F);

            color += BRDF * directionalLight.Color * NL;
        }
        
        for(int i = 0; i < NumPointLights; i++)
        {
            UniformPointLight pointLight = pointLightBuffer.lights[i];
            
            vec3 L = normalize(pointLight.Position - FragPos);
            vec3 H = normalize(V + L);

            float NH = max(0., dot(N, H));
            float NL = max(0., dot(N, L));
            float VH = max(0., dot(V, H));

            vec3 F = FUnreal(VH, F0);

            vec3 fSpec = vec3(D(NH, alpha) * GGX(NL, NV, Roughness) / (4. * NL * NV + MIN_FLT));
            vec3 BRDF = mix(fDiff, fSpec, F);

            color += BRDF * pointLight.Color * NL;
        }

        vec3 ambiant = vec3(0.004) * albedo * AO;

        color += ambiant + Emissive.rgb;

        color = color / (color + 1.);
    }

    color.x = (color.x <= 0.0031308) ? 12.92 * color.x : 1.055 * pow(color.x, 1 / 2.4) - 0.055;
    color.y = (color.y <= 0.0031308) ? 12.92 * color.y : 1.055 * pow(color.y, 1 / 2.4) - 0.055;
    color.z = (color.z <= 0.0031308) ? 12.92 * color.z : 1.055 * pow(color.z, 1 / 2.4) - 0.055;

    outColor = vec4(color, 1.);
}