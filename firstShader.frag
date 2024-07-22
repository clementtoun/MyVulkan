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
	bool useBaseColorTexture;
	bool useMetallicRoughnessTexture;
	bool useNormalTexture;
    vec3 baseColor;
};

layout(set = 2, binding = 1) uniform sampler2D texSampler[3];

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outColor;
layout(location = 3) out vec4 outPbr;

void main() {
    vec3 Normal = useNormalTexture ? normalize(ModelToTangentLocal * (texture(texSampler[2], TexCoord).rgb * 2. - 1.)) : Normal;
    vec4 Color = useBaseColorTexture ? texture(texSampler[0], TexCoord) : vec4(baseColor, 1.);
    vec2 MetallicRoughness = useMetallicRoughnessTexture ? texture(texSampler[1], TexCoord).gb : vec2(roughness, metallic);

    outPosition = vec4(WorldFragPos, 1.);
    outNormal = vec4(Normal, 0.); 
    outColor = Color;
    outPbr = vec4(MetallicRoughness, 0., 0.);
}