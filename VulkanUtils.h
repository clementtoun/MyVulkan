#pragma once

#include "VulkanBase.h"
#include <string>
#include <iostream>

class VulkanUtils
{
public:
	static std::string vkVendorIdToString(uint32_t vendorId);

	static std::string vkPhysicalDeviceTypeToString(VkPhysicalDeviceType physicalDeviceType);

	static std::string queueFlagsToString(VkQueueFlags queueFlags);

	static std::string vkPhysicalDeviceInfoToString(const VkPhysicalDevice& physicalDevice);

	static VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool transferPool);

	static void EndSingleTimeCommands(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkCommandBuffer commandBuffer, bool wait = true);

	static void CopyBuffer(VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	static uint32_t alignedSize(uint32_t size, uint32_t align);

private:
	static std::string boolToString(bool b);
};

