#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <vector>

class RenderPass
{
public:
	RenderPass();

	void cleanup(VkDevice device);

	void addSubPass(VkPipelineBindPoint bindPoint, const std::vector<VkAttachmentReference>& input, 
												   const std::vector<VkAttachmentReference>& color,
												   const std::vector<VkAttachmentReference>& resolve, 
												   const std::vector<VkAttachmentReference>& depthStencil, 
												   const std::vector<uint32_t>& preserve);

	void addSubPassDependency(uint32_t srcSubPass, uint32_t dstSubPass,
		uint32_t srcStageMask, uint32_t dstStageMask, uint32_t srcAccessMask, uint32_t dstAccessMask, VkDependencyFlags dependencyFlags);

	void CreateRenderPass(VkDevice device, const std::vector<VkAttachmentDescription>& attachments);

	VkRenderPass getRenderPass();

private:
	typedef struct s_SubpassDescription
	{
		VkPipelineBindPoint bindPoint;
		std::vector<VkAttachmentReference> input;
		std::vector<VkAttachmentReference> color;
		std::vector<VkAttachmentReference> resolve;
		std::vector<VkAttachmentReference> depthStencil;
		std::vector<uint32_t> preserve;
	} SubpassDescription;

	std::vector<SubpassDescription> m_SubPassDescriptions;
	std::vector <VkSubpassDependency> m_SubpassDependencys;
	VkRenderPass m_RenderPass;
};

