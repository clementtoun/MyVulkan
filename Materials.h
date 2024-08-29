#pragma once

#include "VulkanBase.h"
#include "Image.h"
#include "Descriptor.h"
#include "VkGLM.h"
#include <vector>

struct MaterialUniformBuffer
{
	float metallic;
	float roughness;
	int useBaseColorTexture;
	int useMetallicRoughnessTexture;
	int useNormalTexture;
	int useEmissiveTexture;
	int useAOTexture;
	int padding;
	glm::vec3 baseColor;
};

struct Material
{
	MaterialUniformBuffer materialUniformBuffer;
	std::string baseColorTexturePath;
	TextureImage* baseColorTexture;
	std::string metallicRoughnessTexturePath;
	TextureImage* metallicRoughnessTexture;
	std::string normalTexturePath;
	TextureImage* normalTexture;
	std::string emissiveTexturePath;
	TextureImage* emissiveTexture;
	std::string AOTexturePath;
	TextureImage* AOTexture;
};

class Materials
{
public:
	size_t AddMaterial(Material material);

	void CreateTexures(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void CreatePerMaterialDescriptor(VmaAllocator allocator, VkDevice device);

	VkDescriptorSetLayout GetDescriptorSetsLayout();

	void BindMaterial(size_t index, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	void Cleanup(VmaAllocator allocator, VkDevice device);

private:
	TextureImage* m_defaultTexture;

	std::vector<Material> m_Materials;

	Descriptor m_PerMaterialDescriptor;
};

