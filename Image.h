#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <vector>

class Image
{
public:
	void CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, const std::vector<uint32_t> families, uint32_t mipLevels = 1, uint32_t layer_count = 1, VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED);

	void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

	VkImage GetImage();

	VkImageView GetImageView();

	void Cleanup(VmaAllocator allocator, VkDevice device);

	static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	void TransitionImageLayout(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyBufferToImage(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer buffer, uint32_t width, uint32_t height);

	void generateMipmaps(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, int32_t texWidth, int32_t texHeight);

	uint32_t GetMipLevels();

private:
	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_ImageAllocation;
	VkImageView m_ImageView = VK_NULL_HANDLE;
	uint32_t m_MipLevels = 1;
	uint32_t m_layer_count = 1;
};

class TextureImage
{
public:
	TextureImage(std::string& imagePath, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel);

	TextureImage(unsigned char* pixels, int texWidth, int texHeight, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel);

	void CreateTextureSampler(VkDevice device);

	VkImageView GetImageView();

	VkSampler GetTextureSampler();

	void Cleanup(VmaAllocator allocator, VkDevice device);

private:
	Image m_ImageTexture;
	VkSampler m_TextureSampler = VK_NULL_HANDLE;
};

