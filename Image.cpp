#include "Image.h"
#include "VulkanUtils.h"

void Image::CreateImage(VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, const std::vector<uint32_t> families, uint32_t mipLevels)
{
    m_MipLevels = mipLevels;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = m_MipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t familyCount = static_cast<uint32_t>(families.size());
    imageCreateInfo.queueFamilyIndexCount = familyCount;
    if (familyCount == 1)
    {
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.pQueueFamilyIndices = families.data();
    }
    else
    {
        imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        imageCreateInfo.pQueueFamilyIndices = families.data();
    }


    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.priority = 1.0f;

    if (vmaCreateImage(allocator, &imageCreateInfo, &allocCreateInfo, &m_Image, &m_ImageAllocation, nullptr) != VK_SUCCESS) {
       std::cout << "Image creation failed !" << std::endl;
    }
}

void Image::CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_Image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_MipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
        throw std::runtime_error("échec de la creation de la vue sur une image!");
    }
}

VkImage Image::GetImage()
{
    return m_Image;
}

VkImageView Image::GetImageView()
{
    return m_ImageView;
}

void Image::Cleanup(VmaAllocator allocator, VkDevice device)
{
    if (device != VK_NULL_HANDLE && m_ImageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, m_ImageView, NULL);
    if (device != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, m_Image, NULL);
        vmaFreeMemory(allocator, m_ImageAllocation);
    }
}


VkFormat Image::findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::cout << "No supported format found !" << std::endl;

    return VK_FORMAT_R8G8B8A8_SRGB;
}

VkFormat Image::findDepthFormat(VkPhysicalDevice physicalDevice)
{
    return findSupportedFormat(physicalDevice, 
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void Image::TransitionImageLayout(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, transferPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = m_Image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = m_MipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = 0;
    VkPipelineStageFlags destinationStage = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        std::cout << "Unandled image layout transition !" << std::endl;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    VulkanUtils::EndSingleTimeCommands(device, transferPool, transferQueue, commandBuffer);
}

void Image::CopyBufferToImage(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer buffer, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, transferPool);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        m_Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    VulkanUtils::EndSingleTimeCommands(device, transferPool, transferQueue, commandBuffer);
}

void Image::generateMipmaps(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, int32_t texWidth, int32_t texHeight)
{
    VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, transferPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = m_Image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < m_MipLevels; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    VulkanUtils::EndSingleTimeCommands(device, transferPool, transferQueue, commandBuffer);
}

uint32_t Image::GetMipLevels()
{
    return m_MipLevels;
}

TextureImage::TextureImage(std::string& imagePath, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cout << "Failed to read image !" << std::endl;
        return;
    }

    uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    stagingBufferInfo.queueFamilyIndexCount = 2;
    stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer stagingBuf = {};
    VmaAllocation stagingAlloc = {};
    vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

    void* data;
    vmaMapMemory(allocator, stagingAlloc, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingAlloc);

    stbi_image_free(pixels);

    std::vector<uint32_t> families = { transferFamilyIndice, graphicFamilyIndice };

    uint32_t mipLevels = useMipLevel ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;

    m_ImageTexture.CreateImage(allocator, texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, families, mipLevels);

    m_ImageTexture.TransitionImageLayout(device, transferPool, transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    m_ImageTexture.CopyBufferToImage(device, transferPool, transferQueue, stagingBuf, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    if (mipLevels > 1)
        m_ImageTexture.generateMipmaps(device, graphicPool, graphicQueue, texWidth, texHeight);
    else
        m_ImageTexture.TransitionImageLayout(device, graphicPool, graphicQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuf, nullptr);
    vmaFreeMemory(allocator, stagingAlloc);

    m_ImageTexture.CreateImageView(device, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

TextureImage::TextureImage(unsigned char* pixels, int texWidth, int texHeight, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel)
{
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        std::cout << "Failed to read image !" << std::endl;
        return;
    }

    uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = imageSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    stagingBufferInfo.queueFamilyIndexCount = 2;
    stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    VkBuffer stagingBuf = {};
    VmaAllocation stagingAlloc = {};
    vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

    void* data;
    vmaMapMemory(allocator, stagingAlloc, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingAlloc);

    std::vector<uint32_t> families = { transferFamilyIndice, graphicFamilyIndice };

    uint32_t mipLevels = useMipLevel ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;

    m_ImageTexture.CreateImage(allocator, texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, families, mipLevels);

    m_ImageTexture.TransitionImageLayout(device, transferPool, transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    m_ImageTexture.CopyBufferToImage(device, transferPool, transferQueue, stagingBuf, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    if (mipLevels > 1)
        m_ImageTexture.generateMipmaps(device, graphicPool, graphicQueue, texWidth, texHeight);
    else
        m_ImageTexture.TransitionImageLayout(device, graphicPool, graphicQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuf, nullptr);
    vmaFreeMemory(allocator, stagingAlloc);

    m_ImageTexture.CreateImageView(device, format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void TextureImage::CreateTextureSampler(VkDevice device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 8.;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.;
    samplerInfo.maxLod = static_cast<float>(m_ImageTexture.GetMipLevels());

    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_TextureSampler) != VK_SUCCESS) {
        std::cout << "Failed to create texture sampler!" << std::endl;
    }

}

VkImageView TextureImage::GetImageView()
{
    return m_ImageTexture.GetImageView();
}

VkSampler TextureImage::GetTextureSampler()
{
    return m_TextureSampler;
}

void TextureImage::Cleanup(VmaAllocator allocator, VkDevice device)
{
    if (m_TextureSampler != VK_NULL_HANDLE)
        vkDestroySampler(device, m_TextureSampler, nullptr);

    m_ImageTexture.Cleanup(allocator, device);
}
