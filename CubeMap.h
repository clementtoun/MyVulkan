#pragma once

#include "VulkanBase.h"
#include "VkGLM.h"
#include "VulkanUtils.h"
#include "Image.h"
#include <iostream>
#include <array>

struct VertexPos
{
	glm::vec3 pos;

	static VkVertexInputBindingDescription getVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = sizeof(VertexPos);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertexInputBindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getVertexInputAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 1> vertexInputAttributeDescription;
		vertexInputAttributeDescription[0].location = 0;
		vertexInputAttributeDescription[0].binding = 0;
		vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[0].offset = offsetof(VertexPos, pos);

		return vertexInputAttributeDescription;
	}
};

class CubeMap
{
public:
	CubeMap(std::array<std::string, 6>& imagePaths, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel);

	VkImageView GetImageView();

	void CreateTextureSampler(VkDevice device);

	VkSampler GetTextureSampler();

	void CreateVertexIndexCubeBuffer(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void BindVertexBuffer(VkCommandBuffer commandBuffer);

	void BindIndexBuffer(VkCommandBuffer commandBuffer);

	void Cleanup(VmaAllocator allocator, VkDevice device);

private:
	Image m_ImageTexture;
	VkSampler m_TextureSampler = VK_NULL_HANDLE;
	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_VertexBufferAlloc = nullptr;
	VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_IndexBufferAlloc = nullptr;
};

