#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <vector>
#include <array>

struct StorageBuffer
{
	VkBuffer buffer;
	VmaAllocation memory;
	VmaAllocationInfo memoryInfo;
};

class Descriptor
{
public:
	void CreateDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings, VkDescriptorSetLayoutCreateFlags flags);

	void DestroyDescriptorSetLayout(VkDevice device);

	void AddUniformBuffer(VmaAllocator allocator, VkDeviceSize size);

	void AddStorageBuffer(VmaAllocator allocator, VkDeviceSize size);

	void DestroyStorageBuffer(VmaAllocator allocator, VkDevice device);

	void DestroyUniformBuffer(VmaAllocator allocator, VkDevice device);

	void CreateDescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& descriptorPoolSizes, uint32_t maxSets);

	void DestroyDescriptorPool(VkDevice device);

	void AllocateDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	VkDescriptorSetLayout GetDescriptorSetLayout();

	VkDescriptorPool GetDescriptorPool();

	const std::vector<VkBuffer>& GetUniformBuffers();

	const std::vector<VkDescriptorSet>& GetDescriptorSets();

	const std::vector<VmaAllocation>& GetUniformAllocations();

	const std::vector<VmaAllocationInfo>& GetUniformAllocationInfos();

	const std::vector<StorageBuffer>& GetUniformStorageBuffers();

	void GetUniformStorageBufferAllocationInfos(std::vector<VmaAllocationInfo>& allocationInfos);

private:
	VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	std::vector<VkBuffer> m_UniformBuffers;
	std::vector<VmaAllocation> m_UniformBufferAllocations;
	std::vector<VmaAllocationInfo>m_UniformBufferAllocInfos;
	std::vector<StorageBuffer> m_UniformStorageBuffer;
	std::vector<VkDescriptorSet> m_DescriptorSets;
};

