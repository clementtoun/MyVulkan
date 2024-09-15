#include "CubeMap.h"

CubeMap::CubeMap(std::array<std::string, 6>& imagePaths, VkFormat format, VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandPool graphicPool, VkQueue graphicQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice, bool useMipLevel)
{
    stbi_uc* pixels[6]{};
    VkDeviceSize imageSize = 0;
    int texWidth = 0;
    int texHeight = 0;

    for (int i = 0; i < imagePaths.size(); i++)
    {
        int texChannels;
        pixels[i] = stbi_load(imagePaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize += static_cast<unsigned long long>(texWidth * texHeight * 4);

        if (!pixels[i]) {
            std::cout << "Failed to read image !" << std::endl;
            return;
        }
    }

    const VkDeviceSize layerSize = imageSize / 6;

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

    for (int i = 0; i < imagePaths.size(); i++)
    {
        memcpy(static_cast<stbi_uc*>(data) + layerSize * i, pixels[i], static_cast<size_t>(layerSize));

        stbi_image_free(pixels[i]);
    }

    vmaUnmapMemory(allocator, stagingAlloc);

    std::vector<uint32_t> families = { transferFamilyIndice, graphicFamilyIndice };

    uint32_t mipLevels = useMipLevel ? static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1 : 1;

    m_ImageTexture.CreateImage(allocator, texWidth, texHeight, format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, families, mipLevels, 6);

    m_ImageTexture.TransitionImageLayout(device, transferPool, transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    m_ImageTexture.CopyBufferToImage(device, transferPool, transferQueue, stagingBuf, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    if (mipLevels > 1)
        m_ImageTexture.generateMipmaps(device, graphicPool, graphicQueue, texWidth, texHeight);
    else
        m_ImageTexture.TransitionImageLayout(device, graphicPool, graphicQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuf, nullptr);
    vmaFreeMemory(allocator, stagingAlloc);

    m_ImageTexture.CreateImageView(device, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE);
}

VkImageView CubeMap::GetImageView()
{
    return m_ImageTexture.GetImageView();
}

void CubeMap::CreateTextureSampler(VkDevice device)
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

VkSampler CubeMap::GetTextureSampler()
{
    return m_TextureSampler;
}

void CubeMap::Cleanup(VmaAllocator allocator, VkDevice device)
{
    if (m_TextureSampler != VK_NULL_HANDLE)
        vkDestroySampler(device, m_TextureSampler, nullptr);

    m_ImageTexture.Cleanup(allocator, device);
}
