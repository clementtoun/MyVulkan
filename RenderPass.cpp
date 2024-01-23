#include "RenderPass.h"

RenderPass::RenderPass()
{
	m_RenderPass = VK_NULL_HANDLE;
}

void RenderPass::cleanup(VkDevice device)
{
    m_SubPassDescriptions.clear();
    m_SubpassDependencys.clear();
    
    if (m_RenderPass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device, m_RenderPass, NULL);
}

void RenderPass::addSubPass(VkPipelineBindPoint bindPoint, const std::vector<VkAttachmentReference>& input, 
	const std::vector<VkAttachmentReference>& color, const std::vector<VkAttachmentReference>& resolve, 
	const std::vector<VkAttachmentReference>& depthStencil, const std::vector<uint32_t>& preserve)
{
    SubpassDescription subpassDescription;
    subpassDescription.bindPoint = bindPoint;
    subpassDescription.input = input;
    subpassDescription.color = color;
    subpassDescription.resolve = resolve;
    subpassDescription.depthStencil = depthStencil;
    subpassDescription.preserve = preserve;

    m_SubPassDescriptions.push_back(subpassDescription);
}

void RenderPass::addSubPassDependency(uint32_t srcSubPass, uint32_t dstSubPass, uint32_t srcStageMask, uint32_t dstStageMask, uint32_t srcAccessMask, uint32_t dstAccessMask, VkDependencyFlags dependencyFlags)
{
    VkSubpassDependency subpassDependency;
    subpassDependency.srcSubpass = srcSubPass;
    subpassDependency.dstSubpass = dstSubPass;
    subpassDependency.srcStageMask = srcStageMask;
    subpassDependency.dstStageMask = dstStageMask;
    subpassDependency.srcAccessMask = srcAccessMask;
    subpassDependency.dstAccessMask = dstAccessMask;
    subpassDependency.dependencyFlags = dependencyFlags;

    m_SubpassDependencys.push_back(subpassDependency);
}

void RenderPass::CreateRenderPass(VkDevice device, const std::vector<VkAttachmentDescription>& attachments)
{
    std::vector<VkSubpassDescription> vkSubpassDescriptions;

    for (const auto& subPassDesc : m_SubPassDescriptions)
    {
        VkSubpassDescription vkSubpassDescription;
        vkSubpassDescription.flags = 0;
        vkSubpassDescription.pipelineBindPoint = subPassDesc.bindPoint;
        vkSubpassDescription.inputAttachmentCount = static_cast<uint32_t>(subPassDesc.input.size());
        vkSubpassDescription.pInputAttachments = subPassDesc.input.data();
        vkSubpassDescription.colorAttachmentCount = static_cast<uint32_t>(subPassDesc.color.size());
        vkSubpassDescription.pColorAttachments = subPassDesc.color.data();
        vkSubpassDescription.pResolveAttachments = subPassDesc.resolve.data();
        vkSubpassDescription.pDepthStencilAttachment = subPassDesc.depthStencil.data();
        vkSubpassDescription.preserveAttachmentCount = static_cast<uint32_t>(subPassDesc.preserve.size());
        vkSubpassDescription.pPreserveAttachments = subPassDesc.preserve.data();

        vkSubpassDescriptions.push_back(vkSubpassDescription);
    }

    VkRenderPassCreateInfo renderPassCreateInfo;
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = NULL;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCreateInfo.pAttachments = attachments.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(vkSubpassDescriptions.size());
    renderPassCreateInfo.pSubpasses = vkSubpassDescriptions.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(m_SubpassDependencys.size());
    renderPassCreateInfo.pDependencies = m_SubpassDependencys.data();

    if (vkCreateRenderPass(device, &renderPassCreateInfo, NULL, &m_RenderPass) != VK_SUCCESS)
    {
        std::cout << "Render Pass creation failed !" << std::endl;
    }

    m_SubPassDescriptions.clear();
    m_SubpassDependencys.clear();
}

VkRenderPass RenderPass::getRenderPass()
{
    return m_RenderPass;
}

