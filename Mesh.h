#pragma once

#include <array>
#include <vector>
#include "VulkanBase.h"
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = sizeof(Vertex);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertexInputBindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getVertexInputAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 2> vertexInputAttributeDescription;
		vertexInputAttributeDescription[0].location = 0;
		vertexInputAttributeDescription[0].binding = 0;
		vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[0].offset = offsetof(Vertex, pos);
		vertexInputAttributeDescription[1].location = 1;
		vertexInputAttributeDescription[1].binding = 0;
		vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[1].offset = offsetof(Vertex, color);

		return vertexInputAttributeDescription;
	}
};

class Mesh
{
public:
	Mesh(const std::vector<Vertex>& vertexs, const std::vector<uint32_t>& indexes);

	void DestroyBuffer(VmaAllocator allocator, VkDevice device);

	void SetVertex(const std::vector<Vertex>& vertexs);
	const std::vector<Vertex>& GetVertex();

	void SetIndexes(const std::vector<uint32_t>& indexes);
	const std::vector<uint32_t>& GetIndexes();

	void CreateVertexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void CreateIndexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void BindVertexBuffer(VkCommandBuffer commandBuffer);

	void BindIndexBuffer(VkCommandBuffer commandBuffer);

private:
	void CopyBuffer(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	std::vector<Vertex> m_Vertexs;
	std::vector<uint32_t> m_Indexes;

	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_VertexBufferAlloc;
	VkBuffer m_IndexBuffer;
	VmaAllocation m_IndexBufferAlloc;
};
