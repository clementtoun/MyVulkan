#include "GBuffer.h"

#include <array>

void GBuffer::BuildGBuffer(uint8_t nbImages, uint32_t width, uint32_t height, std::vector<VkImageView> swapChainImageView, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families)
{
	m_GBufferImages.resize(nbImages);

	for (uint8_t i = 0; i < nbImages; i++)
	{
		m_GBufferImages[i].positionImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].positionImageBuffer.CreateImageView(device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].normalImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].normalImageBuffer.CreateImageView(device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].colorImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].colorImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].pbrImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].pbrImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].emissiveImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].emissiveImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void GBuffer::ReBuildGBuffer(uint8_t maxFramesInFlight, uint32_t width, uint32_t height, std::vector<VkImageView> swapChainImageView, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families)
{
	vkDeviceWaitIdle(device);

	Cleanup(allocator, device);

	BuildGBuffer(maxFramesInFlight, width, height, swapChainImageView, allocator, device, families);
}

void GBuffer::Cleanup(VmaAllocator allocator, VkDevice device)
{
	for (auto& GBuffer : m_GBufferImages)
	{
		GBuffer.positionImageBuffer.Cleanup(allocator, device);
		GBuffer.normalImageBuffer.Cleanup(allocator, device);
		GBuffer.colorImageBuffer.Cleanup(allocator, device);
		GBuffer.pbrImageBuffer.Cleanup(allocator, device);
		GBuffer.emissiveImageBuffer.Cleanup(allocator, device);
	}

	m_GBufferImages.clear();
}

std::vector<GBUFFER>& GBuffer::GetGBufferImages()
{
	return m_GBufferImages;
}
