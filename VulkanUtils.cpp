#include "VulkanUtils.h"

std::string VulkanUtils::vkVendorIdToString(uint32_t vendorId)
{
    std::string vendorIdString = "";

    switch (vendorId)
    {
    case VK_VENDOR_ID_VIV:
    {
        vendorIdString = "VIV";
        break;
    }
    case VK_VENDOR_ID_VSI:
    {
        vendorIdString = "VSI";
        break;
    }
    case VK_VENDOR_ID_KAZAN:
    {
        vendorIdString = "KAZAN";
        break;
    }
    case VK_VENDOR_ID_CODEPLAY:
    {
        vendorIdString = "CODEPLAY";
        break;
    }
    case VK_VENDOR_ID_MESA:
    {
        vendorIdString = "MESA";
        break;
    }
    case VK_VENDOR_ID_POCL:
    {
        vendorIdString = "POCL";
        break;
    }
    case VK_VENDOR_ID_MOBILEYE:
    {
        vendorIdString = "MOBILEYE";
        break;
    }
    default:
        vendorIdString = std::to_string(vendorId);
        break;
    }

    return vendorIdString;
}

std::string VulkanUtils::vkPhysicalDeviceTypeToString(VkPhysicalDeviceType physicalDeviceType)
{
    std::string physicalDeviceTypeString = "";

    switch (physicalDeviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
    {
        physicalDeviceTypeString = "OTHER";
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    {
        physicalDeviceTypeString = "INTEGRATED_GPU";
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
    {
        physicalDeviceTypeString = "DISCRETE_GPU";
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
    {
        physicalDeviceTypeString = "VIRTUAL_GPU";
        break;
    }
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
    {
        physicalDeviceTypeString = "TYPE_CPU";
        break;
    }
    default:
    {
        physicalDeviceTypeString = std::to_string(physicalDeviceType);
        break;
    }
    }

    return physicalDeviceTypeString;
}

std::string VulkanUtils::queueFlagsToString(VkQueueFlags queueFlags)
{
    std::string queueFlagsString = "QueueFamilyFlag:";

    if (queueFlags ^ VK_QUEUE_GRAPHICS_BIT)
        queueFlagsString += "\n\tGRAPHICS";
    if (queueFlags ^ VK_QUEUE_COMPUTE_BIT)
        queueFlagsString += "\n\tCOMPUTE";
    if (queueFlags ^ VK_QUEUE_TRANSFER_BIT)
        queueFlagsString += "\n\tTRANSFER";
    if (queueFlags ^ VK_QUEUE_SPARSE_BINDING_BIT)
        queueFlagsString += "\n\tSPARSE_BINDING";
    if (queueFlags ^ VK_QUEUE_PROTECTED_BIT)
        queueFlagsString += "\n\tPROTECTED";
    if (queueFlags ^ VK_QUEUE_VIDEO_DECODE_BIT_KHR)
        queueFlagsString += "\n\tVIDEO_DECODE_KHR";

    return queueFlagsString;
}

std::string VulkanUtils::vkPhysicalDeviceInfoToString(const VkPhysicalDevice& physicalDevice)
{
    std::string log = "-----Physical device info-----";

    if (physicalDevice != VK_NULL_HANDLE)
    {
        VkPhysicalDeviceProperties vkPhysicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &vkPhysicalDeviceProperties);

        log += "\nApiVerison: \n\tMajor: " + std::to_string(VK_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion))
            + "\n\tMinor: " + std::to_string(VK_VERSION_MINOR(vkPhysicalDeviceProperties.apiVersion))
            + "\n\tPatch: " + std::to_string(VK_VERSION_PATCH(vkPhysicalDeviceProperties.apiVersion))
            + "\nDriverVersion: " + std::to_string(vkPhysicalDeviceProperties.driverVersion)
            + "\nVendorID: " + VulkanUtils::vkVendorIdToString(vkPhysicalDeviceProperties.deviceID)
            + "\nDeviceType: " + VulkanUtils::vkPhysicalDeviceTypeToString(vkPhysicalDeviceProperties.deviceType)
            + "\nDeviceName: " + vkPhysicalDeviceProperties.deviceName
            + "\nPipelineCacheUUID: ";
        for (int j = 0; j < VK_UUID_SIZE; j++)
        {
            log += std::to_string(vkPhysicalDeviceProperties.pipelineCacheUUID[j]);
        }
        log += "\nDeviceSparseProperties:\n\tresidencyStandard2DBlockShape: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyStandard2DBlockShape)
        +"\n\tresidencyStandard2DMultisampleBlockShape: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyStandard2DMultisampleBlockShape)
        +"\n\tresidencyStandard3DBlockShape: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyStandard3DBlockShape)
            + "\n\tresidencyAlignedMipSize: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyAlignedMipSize)
        +"\n\tresidencyNonResidentStrict: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict)
        +"\n\tresidencyNonResidentStrict: " + VulkanUtils::boolToString(vkPhysicalDeviceProperties.sparseProperties.residencyNonResidentStrict);
    }
    else
    {
        log += "\nVK_NULL_HANDLE !";
    }

    log += "\n-------------------------";

    return log;
}

VkCommandBuffer VulkanUtils::BeginSingleTimeCommands(VkDevice device, VkCommandPool transferPool)
{
    VkCommandBufferAllocateInfo commandBufferallocInfo{};
    commandBufferallocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferallocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferallocInfo.commandPool = transferPool;
    commandBufferallocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    if (vkAllocateCommandBuffers(device, &commandBufferallocInfo, &commandBuffer) != VK_SUCCESS)
        std::cout << "Failed to allocate single time command buffer !" << "\n";

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        std::cout << "Failed to begin single time command buffer !" << "\n";
    
    return commandBuffer;
}

void VulkanUtils::EndSingleTimeCommands(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandBuffer commandBuffer)
{
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        std::cout << "Failed to end single time command buffer !" << "\n";

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
        std::cout << "Failed to submit single time command buffer !" << "\n";

    VkResult result = vkQueueWaitIdle(transferQueue);
    if (result != VK_SUCCESS)
        std::cout << "Failed to queueWaitIdle single time command buffer ! " << result << "\n";

    vkFreeCommandBuffers(device, transferPool, 1, &commandBuffer);
}

void VulkanUtils::CopyBuffer(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, transferPool);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    VulkanUtils::EndSingleTimeCommands(device, transferPool, transferQueue, commandBuffer);
}

uint32_t VulkanUtils::alignedSize(uint32_t size, uint32_t align)
{
    return ((size - 1) / align + 1) * align;
}

std::string VulkanUtils::boolToString(bool b)
{
    return (b == 0 ? "false" : "true");
}
