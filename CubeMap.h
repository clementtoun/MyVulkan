#pragma once

#include "VulkanBase.h"
#include "Image.h"
#include <iostream>
#include <array>

class CubeMap
{
public:
	CubeMap(std::array<std::string, 6>& imagePaths, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel);

	VkImageView GetImageView();

	void CreateTextureSampler(VkDevice device);

	VkSampler GetTextureSampler();

	void Cleanup(VmaAllocator allocator, VkDevice device);



private:
	Image m_ImageTexture;
	VkSampler m_TextureSampler = VK_NULL_HANDLE;
};

