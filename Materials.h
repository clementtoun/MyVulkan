#pragma once

#include "VulkanBase.h"
#include "Image.h"
#include "Descriptor.h"
#include "VkGLM.h"
#include <vector>

struct alignas(16) MaterialUniformBuffer
{
	alignas(4) float metallic;
	alignas(4) float roughness;
	alignas(4) int useBaseColorTexture;
	alignas(4) int useMetallicRoughnessTexture;
	alignas(4) int useNormalTexture;
	alignas(4) int useEmissiveTexture;
	alignas(4) int useAOTexture;
	alignas(16) glm::vec3 baseColor;
	alignas(16) glm::vec3 emissiveColor;
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

	std::vector<Material>& GetMaterials();

	void Cleanup(VmaAllocator allocator, VkDevice device);

private:
	TextureImage* m_defaultTexture;

	std::vector<Material> m_Materials;

	Descriptor m_PerMaterialDescriptor;
};

