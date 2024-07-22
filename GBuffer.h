#pragma once

#include "VulkanBase.h"
#include "Image.h"

typedef struct s_GBUFFER
{
	Image positionImageBuffer;
	Image normalImageBuffer;
	Image colorImageBuffer;
	Image pbrImageBuffer; //R: roughness, G: metallic, B: emissive, A: undefined
} GBUFFER;

class GBuffer
{
public:
	void BuildGBuffer(uint8_t nbImages, uint32_t width, uint32_t height, VkRenderPass renderPass, std::vector<VkImageView> swapChainImageView, VkImageView depthAttachment, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families);

	void ReBuildGBuffer(uint8_t nbImages, uint32_t width, uint32_t height, VkRenderPass renderPass, std::vector<VkImageView> swapChainImageView, VkImageView depthAttachment, VmaAllocator allocator, VkDevice device, const std::vector<uint32_t> families);

	void Cleanup(VmaAllocator allocator, VkDevice device);

	std::vector<GBUFFER>& GetGBufferImages();

private:
	std::vector<GBUFFER> m_GBufferImages;
};

