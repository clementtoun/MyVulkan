#include "Materials.h"

size_t Materials::AddMeterial(glm::vec3 baseColor, float metallic, float roughness, std::string baseColorTexturePath, std::string metallicRoughnessTexturePath, std::string normalTexturePath)
{
	Material material;
	material.materialUniformBuffer.baseColor = baseColor;
	material.materialUniformBuffer.metallic = metallic;
	material.materialUniformBuffer.roughness = roughness;
	material.baseColorTexturePath = baseColorTexturePath;
	material.metallicRoughnessTexturePath = metallicRoughnessTexturePath;
	material.normalTexturePath = normalTexturePath;
	material.baseColorTexture = nullptr;
	material.metallicRoughnessTexture = nullptr;
	material.normalTexture = nullptr;
	material.materialUniformBuffer.useBaseColorTexture = false;
	material.materialUniformBuffer.useMetallicRoughnessTexture = false;
	material.materialUniformBuffer.useNormalTexture = false;

	m_Materials.push_back(material);

	return m_Materials.size() - 1;
}

void Materials::CreateTexures(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice)
{
	for (auto& material : m_Materials)
	{
		if (material.baseColorTexturePath != "" && material.baseColorTexture == nullptr)
		{
			material.baseColorTexture = new TextureImage(material.baseColorTexturePath, VK_FORMAT_R8G8B8A8_SRGB, allocator, device, transferPool, transferQueue, graphicPool, graphicQueue, transferFamilyIndice, graphicFamilyIndice);
			material.baseColorTexture->CreateTextureSampler(device);
			material.materialUniformBuffer.useBaseColorTexture = true;
		}
		if (material.metallicRoughnessTexturePath != "" && material.metallicRoughnessTexture == nullptr)
		{
			material.metallicRoughnessTexture = new TextureImage(material.metallicRoughnessTexturePath, VK_FORMAT_R8G8B8A8_UNORM, allocator, device, transferPool, transferQueue, graphicPool, graphicQueue, transferFamilyIndice, graphicFamilyIndice);
			material.metallicRoughnessTexture->CreateTextureSampler(device);
			material.materialUniformBuffer.useMetallicRoughnessTexture = true;
		}
		if (material.normalTexturePath != "" && material.normalTexture == nullptr)
		{
			material.normalTexture = new TextureImage(material.normalTexturePath, VK_FORMAT_R8G8B8A8_UNORM, allocator, device, transferPool, transferQueue, graphicPool, graphicQueue, transferFamilyIndice, graphicFamilyIndice);
			material.normalTexture->CreateTextureSampler(device);
			material.materialUniformBuffer.useNormalTexture = true;
		}
	}

	unsigned char* pixels = (unsigned char*) malloc(sizeof(unsigned char) * 64);

	m_defaultTexture = new TextureImage(pixels, 4, 4, VK_FORMAT_R8G8B8A8_SRGB, allocator, device, transferPool, transferQueue, graphicPool, graphicQueue, transferFamilyIndice, graphicFamilyIndice);
	m_defaultTexture->CreateTextureSampler(device);

	free(pixels);
}

void Materials::CreatePerMaterialDescriptor(VmaAllocator allocator, VkDevice device)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.pImmutableSamplers = NULL;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 3;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = NULL;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	m_PerMaterialDescriptor.CreateDescriptorSetLayout(device, { uboLayoutBinding, samplerLayoutBinding }, 0);

	auto layout = m_PerMaterialDescriptor.GetDescriptorSetLayout();
	std::vector<VkDescriptorSetLayout> layouts;

	for (auto& material : m_Materials)
	{
		m_PerMaterialDescriptor.AddUniformBuffer(allocator, device, sizeof(MaterialUniformBuffer));
		layouts.push_back(layout);
	}

	std::vector<VkDescriptorPoolSize> poolSizes{};
	VkDescriptorPoolSize poolSize{};

	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_Materials.size());
	poolSizes.push_back(poolSize);

	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = static_cast<uint32_t>(m_Materials.size());
	poolSizes.push_back(poolSize);

	m_PerMaterialDescriptor.CreateDescriptorPool(device, poolSizes, m_Materials.size());

	m_PerMaterialDescriptor.AllocateDescriptorSet(device, layouts);

	auto uniformBuffers = m_PerMaterialDescriptor.GetUniformBuffers();
	auto descriptorSets = m_PerMaterialDescriptor.GetDescriptorSets();
	auto uniformAllocInfo = m_PerMaterialDescriptor.GetUniformAllocationInfos();

	for (int i = 0; i < m_Materials.size(); i++)
	{
		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(MaterialUniformBuffer);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		VkDescriptorImageInfo imageColorInfo{};
		imageColorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageColorInfo.imageView = m_Materials[i].materialUniformBuffer.useBaseColorTexture ? m_Materials[i].baseColorTexture->GetImageView() : m_defaultTexture->GetImageView();
		imageColorInfo.sampler = m_Materials[i].materialUniformBuffer.useBaseColorTexture ? m_Materials[i].baseColorTexture->GetTextureSampler() : m_defaultTexture->GetTextureSampler();

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageColorInfo;

		VkDescriptorImageInfo imageMetallicRoughnessInfo{};
		imageMetallicRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMetallicRoughnessInfo.imageView = m_Materials[i].materialUniformBuffer.useMetallicRoughnessTexture ? m_Materials[i].metallicRoughnessTexture->GetImageView() : m_defaultTexture->GetImageView();
		imageMetallicRoughnessInfo.sampler = m_Materials[i].materialUniformBuffer.useMetallicRoughnessTexture ? m_Materials[i].metallicRoughnessTexture->GetTextureSampler() : m_defaultTexture->GetTextureSampler();

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = descriptorSets[i];
		descriptorWrites[2].dstBinding = 1;
		descriptorWrites[2].dstArrayElement = 1;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &imageMetallicRoughnessInfo;

		VkDescriptorImageInfo imageNormalInfo{};
		imageNormalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageNormalInfo.imageView = m_Materials[i].materialUniformBuffer.useNormalTexture ? m_Materials[i].normalTexture->GetImageView() : m_defaultTexture->GetImageView();
		imageNormalInfo.sampler = m_Materials[i].materialUniformBuffer.useNormalTexture ? m_Materials[i].normalTexture->GetTextureSampler() : m_defaultTexture->GetTextureSampler();

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = descriptorSets[i];
		descriptorWrites[3].dstBinding = 1;
		descriptorWrites[3].dstArrayElement = 2;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &imageNormalInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		memcpy(uniformAllocInfo[i].pMappedData, &(m_Materials[i].materialUniformBuffer), sizeof(MaterialUniformBuffer));
	}
}

VkDescriptorSetLayout Materials::GetDescriptorSetsLayout()
{
	return m_PerMaterialDescriptor.GetDescriptorSetLayout();
}

void Materials::BindMaterial(size_t index, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &m_PerMaterialDescriptor.GetDescriptorSets()[index], 0, NULL);
}

void Materials::Cleanup(VmaAllocator allocator, VkDevice device)
{
	m_PerMaterialDescriptor.DestroyDescriptorPool(device);
	m_PerMaterialDescriptor.DestroyDescriptorSetLayout(device);
	m_PerMaterialDescriptor.DestroyUniformBuffer(allocator, device);

	for (auto& material : m_Materials)
	{
		if (material.baseColorTexture)
		{
			material.baseColorTexture->Cleanup(allocator, device);
			delete material.baseColorTexture;
		}
		if (material.metallicRoughnessTexture)
		{
			material.metallicRoughnessTexture->Cleanup(allocator, device);
			delete material.metallicRoughnessTexture;
		}
		if (material.normalTexture)
		{
			material.normalTexture->Cleanup(allocator, device);
			delete material.normalTexture;
		}
	}

	m_defaultTexture->Cleanup(allocator, device);
	delete m_defaultTexture;

	m_Materials.clear();
}
