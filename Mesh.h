#pragma once

#include <array>
#include <vector>
#include "VulkanBase.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	uint8_t color[3];

	static VkVertexInputBindingDescription getVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = sizeof(Vertex);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertexInputBindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getVertexInputAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 3> vertexInputAttributeDescription;
		vertexInputAttributeDescription[0].location = 0;
		vertexInputAttributeDescription[0].binding = 0;
		vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[0].offset = offsetof(Vertex, pos);
		vertexInputAttributeDescription[1].location = 1;
		vertexInputAttributeDescription[1].binding = 0;
		vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[1].offset = offsetof(Vertex, normal);
		vertexInputAttributeDescription[2].location = 2;
		vertexInputAttributeDescription[2].binding = 0;
		vertexInputAttributeDescription[2].format = VK_FORMAT_R8G8B8_UNORM;
		vertexInputAttributeDescription[2].offset = offsetof(Vertex, color);

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

	const glm::mat4& GetModel();

	void SetModel(const glm::mat4 model);

	void AutoComputeNormals();

private:
	void CopyBuffer(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	std::vector<Vertex> m_Vertexs;
	std::vector<uint32_t> m_Indexes;

	glm::mat4 m_Model;

	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_VertexBufferAlloc;
	VkBuffer m_IndexBuffer;
	VmaAllocation m_IndexBufferAlloc;
};

