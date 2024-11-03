#include "SwapChain.h"
#include "Renderer.h"
#include "Image.h"

#include <algorithm>

SwapChain::SwapChain()
{
    m_SurfaceFormat = VK_FORMAT_UNDEFINED;
    m_PresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
    m_Extent = { 0, 0 };
}

void SwapChain::BuildSwapChain(VkPhysicalDevice physicalDevice, VmaAllocator allocator, VkDevice device, VkSurfaceKHR surface, GLFWwindow * window, const QueueFamilyIndices & queueFamilyIndices)
{
    SwapChainSupportDetails swapChainSupport = QuerySupportDetails(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndicesNeeded[] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };

    if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicesNeeded;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optionnel
        createInfo.pQueueFamilyIndices = nullptr; // Optionnel
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_SwapchainKHR) != VK_SUCCESS) 
    {
        std::cout << "SwapChain creation failed !" << std::endl;
    }

    m_SurfaceFormat = surfaceFormat.format;
    m_Extent = extent;

    vkGetSwapchainImagesKHR(device, m_SwapchainKHR, &imageCount, nullptr);
    m_Images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, m_SwapchainKHR, &imageCount, m_Images.data());

    m_ImageViews.resize(m_Images.size());

    for (int i = 0; i < m_ImageViews.size(); i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = NULL;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.image = m_Images[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = m_SurfaceFormat;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &m_ImageViews[i]) != VK_SUCCESS)
        {
            std::cout << "Image view " << i << " creation failed !" << std::endl;
        }
    }

    m_RTImages.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_RTImages[i].CreateImage(allocator, extent.width, extent.height, surfaceFormat.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, { queueFamilyIndices.graphicsFamily.value()});
        m_RTImages[i].CreateImageView(device, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChain::RebuildSwapChain(VkPhysicalDevice physicalDevice, VmaAllocator allocator, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, const QueueFamilyIndices& queueFamilyIndices)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    Cleanup(device, allocator);

    BuildSwapChain(physicalDevice, allocator, device, surface, window, queueFamilyIndices);
}

void SwapChain::CreateFramebuffer(VkDevice device, VkRenderPass renderPass, VkRenderPass finalRenderPass, GBuffer& GBuffer, VkImageView depthAttachment)
{
    m_Framebuffers.resize(MAX_FRAMES_IN_FLIGHT);

    auto GBufferImages = GBuffer.GetGBufferImages();

    for (int i = 0; i < m_Framebuffers.size(); i++)
    {
        std::array<VkImageView, 7> attachments = { 
            GBufferImages[i].positionImageBuffer.GetImageView(),  
            GBufferImages[i].normalImageBuffer.GetImageView(),
            GBufferImages[i].colorImageBuffer.GetImageView(),
            GBufferImages[i].pbrImageBuffer.GetImageView(),
            GBufferImages[i].emissiveImageBuffer.GetImageView(),
            depthAttachment, m_RTImages[i].GetImageView()};

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = NULL;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = m_Extent.width;
        framebufferCreateInfo.height = m_Extent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, &m_Framebuffers[i]) != VK_SUCCESS)
        {
            std::cout << "Framebuffer creation failed !" << std::endl;
        }
    }

    m_FinalFramebuffers.resize(m_Images.size());
    for (int i = 0; i < m_FinalFramebuffers.size(); i++)
    {
        std::array<VkImageView, 1> attachments = {
            m_ImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreateInfo;
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = NULL;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = finalRenderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = m_Extent.width;
        framebufferCreateInfo.height = m_Extent.height;
        framebufferCreateInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferCreateInfo, NULL, &m_FinalFramebuffers[i]) != VK_SUCCESS)
        {
            std::cout << "Framebuffer creation failed !" << std::endl;
        }
    }
}

void SwapChain::Cleanup(VkDevice device, VmaAllocator allocator)
{
    for (int i = 0; i < m_Framebuffers.size(); i++)
        vkDestroyFramebuffer(device, m_Framebuffers[i], NULL);

    m_Framebuffers.clear();

    for (int i = 0; i < m_FinalFramebuffers.size(); i++)
        vkDestroyFramebuffer(device, m_FinalFramebuffers[i], NULL);

    m_FinalFramebuffers.clear();

    for (int i = 0; i < m_ImageViews.size(); i++)
    {
        vkDestroyImageView(device, m_ImageViews[i], NULL);
    }

    m_ImageViews.clear();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (m_Sampler.size() > 0)
            vkDestroySampler(device, m_Sampler[i], NULL);
        if (m_ImGuiDescriptor.size() > 0 && m_ImGuiDescriptor[i] != VK_NULL_HANDLE)
            ImGui_ImplVulkan_RemoveTexture(m_ImGuiDescriptor[i]);

        m_RTImages[i].Cleanup(allocator, device);
    }
    m_RTImages.clear();
    m_Sampler.clear();
    m_ImGuiDescriptor.clear();

    if (device != VK_NULL_HANDLE && m_SwapchainKHR != VK_NULL_HANDLE)
        vkDestroySwapchainKHR(device, m_SwapchainKHR, NULL);

    m_Images.clear();
}

VkSwapchainKHR SwapChain::GetSwapChain()
{
    return m_SwapchainKHR;
}

const VkFormat& SwapChain::GetSurfaceFormat()
{
    return m_SurfaceFormat;
}

const VkExtent2D& SwapChain::GetExtent()
{
    return m_Extent;
}

const std::vector<VkImage> SwapChain::GetImages()
{
    std::vector<VkImage> images;
    images.resize(m_RTImages.size());

    for (int i = 0; i < m_RTImages.size(); i++)
    {
        images[i] = m_RTImages[i].GetImage();
    }

    return images;
}

const std::vector<VkImageView> SwapChain::GetImageViews()
{
    std::vector<VkImageView> imageViews;
    imageViews.resize(m_RTImages.size());

    for (int i = 0; i < m_RTImages.size(); i++)
    {
        imageViews[i] = m_RTImages[i].GetImageView();
    }

    return imageViews;
}

const std::vector<VkFramebuffer>& SwapChain::GetFramebuffers()
{
    return m_Framebuffers;
}

const std::vector<VkImage>& SwapChain::GetFinalImage()
{
    return m_Images;
}

const std::vector<VkImageView>& SwapChain::GetFinalImageViews()
{
    return m_ImageViews;
}

const std::vector<VkFramebuffer>& SwapChain::GetFinalFramebuffers()
{
    return m_FinalFramebuffers;
}

void SwapChain::CreateImGuiImageDescriptor(VkDevice device)
{
    m_Sampler.resize(m_RTImages.size());
    m_ImGuiDescriptor.resize(m_RTImages.size());
    for (int i = 0; i < m_RTImages.size(); i++)
    {
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // outside image bounds just use border color
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.minLod = -1000;
        sampler_info.maxLod = 1000;
        sampler_info.maxAnisotropy = 1.0f;

        vkCreateSampler(device, &sampler_info, nullptr, &m_Sampler[i]);

        m_ImGuiDescriptor[i] = ImGui_ImplVulkan_AddTexture(m_Sampler[i], m_RTImages[i].GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

const VkDescriptorSet& SwapChain::GetImGuiImageDescriptor(uint32_t i)
{
    return m_ImGuiDescriptor[i];
}

SwapChainSupportDetails SwapChain::QuerySupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == PRESENT_MODE) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}
