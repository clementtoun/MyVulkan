#include "Mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertexs, const std::vector<uint32_t>& indexes)
{
	m_Vertexs = vertexs;
	m_Indexes = indexes;
}

void Mesh::DestroyBuffer(VmaAllocator allocator, VkDevice device)
{
	if (m_VertexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, m_VertexBuffer, NULL);
		vmaFreeMemory(allocator, m_VertexBufferAlloc);
	}

	if (m_IndexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, m_IndexBuffer, NULL);
		vmaFreeMemory(allocator, m_IndexBufferAlloc);
	}
}

void Mesh::SetVertex(const std::vector<Vertex>& vertexs)
{
	m_Vertexs.clear();
	m_Vertexs = vertexs;
}

const std::vector<Vertex>& Mesh::GetVertex()
{
	return m_Vertexs;
}

void Mesh::SetIndexes(const std::vector<uint32_t>& indexes)
{
	m_Indexes.clear();
	m_Indexes = indexes;
}

const std::vector<uint32_t>& Mesh::GetIndexes()
{
	return m_Indexes;
}

void Mesh::CreateVertexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice)
{
	uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

	VkDeviceSize vertexBufferSize = m_Vertexs.size() * sizeof(Vertex);

	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = vertexBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	stagingBufferInfo.queueFamilyIndexCount = 2;
	stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuf = {};
	VmaAllocation stagingAlloc = {};
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

	void* data;
	vmaMapMemory(allocator, stagingAlloc, &data);
	memcpy(data, m_Vertexs.data(), (size_t)vertexBufferSize);
	vmaUnmapMemory(allocator, stagingAlloc);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = vertexBufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_VertexBuffer, &m_VertexBufferAlloc, NULL);

	CopyBuffer(device, transferPool, transferQueue, stagingBuf, m_VertexBuffer, vertexBufferSize);

	vkDestroyBuffer(device, stagingBuf, nullptr);
	vmaFreeMemory(allocator, stagingAlloc);
}

void Mesh::CreateIndexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice)
{
	uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

	VkDeviceSize indexBufferSize = m_Indexes.size() * sizeof(uint32_t);

	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = indexBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	stagingBufferInfo.queueFamilyIndexCount = 2;
	stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuf = {};
	VmaAllocation stagingAlloc = {};

	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

	void* data;
	vmaMapMemory(allocator, stagingAlloc, &data);
	memcpy(data, m_Indexes.data(), (size_t)indexBufferSize);
	vmaUnmapMemory(allocator, stagingAlloc);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = indexBufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_IndexBuffer, &m_IndexBufferAlloc, NULL);

	CopyBuffer(device, transferPool, transferQueue, stagingBuf, m_IndexBuffer, indexBufferSize);

	vkDestroyBuffer(device, stagingBuf, nullptr);
	vmaFreeMemory(allocator, stagingAlloc);
}

void Mesh::BindVertexBuffer(VkCommandBuffer commandBuffer)
{
	VkBuffer vertexBuffers[] = { m_VertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void Mesh::BindIndexBuffer(VkCommandBuffer commandBuffer)
{
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::CopyBuffer(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo commandBufferallocInfo{};
	commandBufferallocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferallocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferallocInfo.commandPool = transferPool;
	commandBufferallocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &commandBufferallocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, transferPool, 1, &commandBuffer);
}
