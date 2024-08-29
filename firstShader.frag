#version 450

layout(location = 0) in vec3 Normal;
layout(location = 1) in vec2 TexCoord;
layout(location = 2) in vec3 FragColor;
layout(location = 3) in vec3 WorldFragPos;
layout(location = 4) in mat3 ModelToTangentLocal;

layout(set = 2, binding = 0) uniform Material
{
	float metallic;                       
    float roughness;                      
    int useBaseColorTexture;              
    int useMetallicRoughnessTexture;      
    int useNormalTexture;  
    int useEmissiveTexture;
    int useAOTexture;   
    int padding;
    vec3 baseColor;                                  
};

layout(set = 2, binding = 1) uniform sampler2D texSampler[5];

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec4 outPbr;
layout(location = 4) out vec4 outEmissive;

void main() {
    vec3 Normal = useNormalTexture == 1 ? normalize(ModelToTangentLocal * (texture(texSampler[2], TexCoord).rgb * 2. - 1.)) : Normal;
    vec4 Color = useBaseColorTexture == 1 ? texture(texSampler[0], TexCoord) : vec4(baseColor, 1.);

    vec3 PbrAO = texture(texSampler[1], TexCoord).rgb;
    vec2 MetallicRoughness = useMetallicRoughnessTexture == 1 ? texture(texSampler[1], TexCoord).gb : vec2(roughness, metallic);

    vec3 Emissive = useEmissiveTexture == 1 ? texture(texSampler[3], TexCoord).rgb : vec3(0.);

    float AO = useAOTexture == 1 ? texture(texSampler[4], TexCoord).r : 1.;

    outPosition = vec4(WorldFragPos, 1.);
    outNormal = vec4(Normal, 0.); 
    outColor = Color;
    outPbr = vec4(MetallicRoughness, AO, 0.);
    outEmissive = vec4(Emissive, 0.);
}