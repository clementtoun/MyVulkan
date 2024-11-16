#include "GBuffer.h"

#include <array>

void GBuffer::BuildGBuffer(uint8_t nbImages, uint32_t width, uint32_t height, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families)
{
	m_GBufferImages.resize(nbImages);

	for (uint8_t i = 0; i < nbImages; i++)
	{
		m_GBufferImages[i].positionImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].positionImageBuffer.CreateImageView(device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
		
		m_GBufferImages[i].normalImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].normalImageBuffer.CreateImageView(device, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].colorImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].colorImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].pbrImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].pbrImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		m_GBufferImages[i].emissiveImageBuffer.CreateImage(allocator, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, families);
		m_GBufferImages[i].emissiveImageBuffer.CreateImageView(device, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 0.;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.;
		samplerInfo.maxLod = 0;

		if (vkCreateSampler(device, &samplerInfo, nullptr, &m_GBufferImages[i].sampler) != VK_SUCCESS) {
			std::cout << "Failed to create texture sampler!\n";
		}
	}
}

void GBuffer::ReBuildGBuffer(uint8_t maxFramesInFlight, uint32_t width, uint32_t height, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families)
{
	vkDeviceWaitIdle(device);

	Cleanup(allocator, device);

	BuildGBuffer(maxFramesInFlight, width, height, allocator, device, families);
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

		vkDestroySampler(device, GBuffer.sampler, nullptr);
	}

	m_GBufferImages.clear();
}

std::vector<GBUFFER>& GBuffer::GetGBufferImages()
{
	return m_GBufferImages;
}
