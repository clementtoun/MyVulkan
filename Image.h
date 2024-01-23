#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <vector>

class Image
{
public:
	void CreateImage(VmaAllocator allocator, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, const std::vector<uint32_t> families);

	void CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectFlags);

	VkImage GetImage();

	VkImageView GetImageView();

	void Cleanup(VmaAllocator allocator, VkDevice device);

	static VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	static VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);

	VkImage m_Image = VK_NULL_HANDLE;
	VmaAllocation m_ImageAllocation;
	VkImageView m_ImageView = VK_NULL_HANDLE;
};

