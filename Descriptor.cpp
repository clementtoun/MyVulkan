#include "Descriptor.h"

void Descriptor::CreateDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& descriptorSetLayoutBindings, VkDescriptorSetLayoutCreateFlags flags)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = NULL;
	descriptorSetLayoutCreateInfo.flags = flags;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, NULL, &m_DescriptorSetLayout) != VK_SUCCESS)
		std::cout << "DescriptorSetLayout creation failed !" << std::endl;
}

void Descriptor::DestroyDescriptorSetLayout(VkDevice device)
{
	if (device != VK_NULL_HANDLE && m_DescriptorSetLayout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, NULL);
}

void Descriptor::AddUniformBuffer(VmaAllocator allocator, VkDevice device, VkDeviceSize size)
{
	VkBufferCreateInfo uniformBufferCreateInfo{};
	uniformBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	uniformBufferCreateInfo.size = size;
	uniformBufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

	VmaAllocationCreateInfo uniformAllocInfo{};
	uniformAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	uniformAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer uniformBuffer;
	VmaAllocation uniformBufferAllocation;
	if (vmaCreateBuffer(allocator, &uniformBufferCreateInfo, &uniformAllocInfo, &uniformBuffer, &uniformBufferAllocation, NULL) != VK_SUCCESS)
		std::cout << "Uniform buffer allocation failed !" << std::endl;

	m_uniformBuffers.push_back(uniformBuffer);
	m_uniformBufferAllocations.push_back(uniformBufferAllocation);
}

void Descriptor::DestroyUniformBuffer(VmaAllocator allocator, VkDevice device)
{
	if (device != VK_NULL_HANDLE)
	{
		for (int i = 0; i < m_uniformBuffers.size(); i++)
		{
			vkDestroyBuffer(device, m_uniformBuffers[i], NULL);
			vmaFreeMemory(allocator, m_uniformBufferAllocations[i]);
		}
	}
}

void Descriptor::CreateDescriptorPool(VkDevice device, const std::vector<VkDescriptorPoolSize>& descriptorPoolSizes, uint32_t maxSets)
{
	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
	poolInfo.pPoolSizes = descriptorPoolSizes.data();
	poolInfo.maxSets = maxSets;

	if (vkCreateDescriptorPool(device, &poolInfo, NULL, &m_DescriptorPool) != VK_SUCCESS)
		std::cout << "DesciptorPool creation failed !" << std::endl;
}

void Descriptor::DestroyDescriptorPool(VkDevice device)
{
	if (device != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
}

void Descriptor::AllocateDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	uint32_t descriptorCount = static_cast<uint32_t>(descriptorSetLayouts.size());

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = descriptorCount;
	allocInfo.pSetLayouts = descriptorSetLayouts.data();

	m_DescriptorSets.resize(descriptorCount);

	if (vkAllocateDescriptorSets(device, &allocInfo, m_DescriptorSets.data()) != VK_SUCCESS)
		std::cout << "DescriptorSet allocation failed !" << std::endl;
}

VkDescriptorSetLayout Descriptor::GetDescriptorSetLayout()
{
	return m_DescriptorSetLayout;
}

VkDescriptorPool Descriptor::GetDescriptorPool()
{
	return m_DescriptorPool;
}

const std::vector<VkBuffer>& Descriptor::GetUniformBuffers()
{
	return m_uniformBuffers;
}

const std::vector<VkDescriptorSet>& Descriptor::GetDescriptorSets()
{
	return m_DescriptorSets;
}

const std::vector<VmaAllocation>& Descriptor::GetUniformAllocation()
{
	return m_uniformBufferAllocations;
}
