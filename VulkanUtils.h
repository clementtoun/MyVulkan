#pragma once

#include "VulkanBase.h"
#include <string>

class VulkanUtils
{
public:
	static std::string vkVendorIdToString(uint32_t vendorId);

	static std::string vkPhysicalDeviceTypeToString(VkPhysicalDeviceType physicalDeviceType);

	static std::string queueFlagsToString(VkQueueFlags queueFlags);

	static std::string vkPhysicalDeviceInfoToString(const VkPhysicalDevice& physicalDevice);

private:
	static std::string boolToString(bool b);
};

