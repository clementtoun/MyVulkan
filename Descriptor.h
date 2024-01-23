#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <vector>
#include <array>

class Descriptor
{
public:
	void CreateDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings, VkDescriptorSetLayoutCreateFlags flags);

	void DestroyDescriptorSetLayout(VkDevice device);

	void AddUniformBuffer(VmaAllocator allocator, VkDevice device, VkDeviceSize size);

	void DestroyUniformBuffer(VmaAllocator allocator, VkDevice device);

	void CreateDescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& descriptorPoolSizes, uint32_t maxSets);

	void DestroyDescriptorPool(VkDevice device);

	void AllocateDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	VkDescriptorSetLayout GetDescriptorSetLayout();

	VkDescriptorPool GetDescriptorPool();

	const std::vector<VkBuffer>& GetUniformBuffers();

	const std::vector<VkDescriptorSet>& GetDescriptorSets();

	const std::vector<VmaAllocation>& GetUniformAllocation();

private:
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VmaAllocation> m_uniformBufferAllocations;
	std::vector<VkDescriptorSet> m_DescriptorSets;
};

