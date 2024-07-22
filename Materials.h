#pragma once

#include "VulkanBase.h"
#include "Image.h"
#include "Descriptor.h"
#include <vector>

struct MaterialUniformBuffer
{
	float metallic;
	float roughness;
	bool useBaseColorTexture;
	bool useMetallicRoughnessTexture;
	bool useNormalTexture;
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
};

class Materials
{
public:
	size_t AddMeterial(glm::vec3 baseColor, float metallic, float roughness, std::string baseColorTexturePath, std::string metallicRoughnessTexturePath, std::string normalTexturePath);

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

