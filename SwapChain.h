#pragma once

#include "VulkanBase.h"
#include "QueueVulkan.h"
#include "GBuffer.h"
#include <iostream>
#include <vector>
#include <array>

//#define PRESENT_MODE VK_PRESENT_MODE_FIFO_KHR
#define PRESENT_MODE VK_PRESENT_MODE_MAILBOX_KHR

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain
{
public:
	SwapChain();

	void BuildSwapChain(VkPhysicalDevice physicalDevice, VmaAllocator allocator, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, const QueueFamilyIndices& queueFamilyIndices);

	void RebuildSwapChain(VkPhysicalDevice physicalDevice, VmaAllocator allocator, VkDevice device, VkSurfaceKHR surface, GLFWwindow* window, const QueueFamilyIndices& queueFamilyIndices);

	void CreateFramebuffer(VkDevice device, VkRenderPass renderPass, VkRenderPass finalRenderPass, GBuffer& GBuffer, VkImageView depthAttachment);

	SwapChainSupportDetails QuerySupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface);

	void Cleanup(VkDevice device, VmaAllocator allocator);

	VkSwapchainKHR GetSwapChain();

	const VkFormat& GetSurfaceFormat();

	const VkExtent2D& GetExtent();

	const void GetImages(std::vector<VkImage>& outImages);

	const void GetImageViews(std::vector<VkImageView>& outImageViews);

	const std::vector<VkFramebuffer>& GetFramebuffers();

	const std::vector<VkImage>& GetFinalImage();

	const std::vector<VkImageView>& GetFinalImageViews();

	const std::vector<VkFramebuffer>& GetFinalFramebuffers();

	void CreateImGuiImageDescriptor(VkDevice device);

	const VkDescriptorSet& GetImGuiImageDescriptor(uint32_t i);

private:

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

	VkFormat m_SurfaceFormat;
	VkPresentModeKHR m_PresentMode;
	VkExtent2D m_Extent;
	VkSwapchainKHR m_SwapchainKHR = VK_NULL_HANDLE;
	std::vector<VkImage> m_Images;
	std::vector<VkImageView> m_ImageViews;
	std::vector<Image> m_RTImages;
	std::vector<VkSampler> m_Sampler;
	std::vector<VkDescriptorSet> m_ImGuiDescriptor;
	std::vector<VkFramebuffer> m_Framebuffers;
	std::vector<VkFramebuffer> m_FinalFramebuffers;
};

