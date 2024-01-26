#pragma once

#include "VulkanBase.h"
#include "VulkanUtils.h"
#include "GlfwWindow.h"
#include "QueueVulkan.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Mesh.h"
#include "Image.h"
#include "Descriptor.h"
#include "Camera.h"
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <map>

const std::string validationLayers = "VK_LAYER_KHRONOS_validation";

const std::vector<std::string> wantedLayers = {};

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#define MAX_FRAMES_IN_FLIGHT 3

typedef struct s_MVP
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
} MVP;

class Renderer
{

public:
	Renderer(const std::string& ApplicationName = "DefaultApplication", uint32_t ApplicationVersion = 0, const std::string& EngineName = "DefaultEngine", uint32_t EngineVersion = 0, int width = 1080, int height = 720);

	~Renderer();

	void CreateWindow(const std::string& WindowName, int width, int height);

	void CreateInstance(const std::string& ApplicationName, uint32_t ApplicationVersion, const std::string& EngineName, uint32_t EngineVersion);

	void CreateSurface();

	void SelectPhysicalDevice();

	void CreateVmaAllocator();

    void FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

	void CreateLogicalDevice();

	void CreateDepthRessources();

	void CreateRenderPass();

	void CreatePerPassDescriptor();

	void CreatePerMeshDescriptor();

	void CreateGraphicPipeline(Shader& vertexShader, Shader& fragmentShader);

	void CreateCommandPool();

	void CreateCommandBuffers();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void CleanupCommandBuffers();

	void CreateSyncObject();

	bool WindowShouldClose();

	void Draw();

	void UpdateUniform();

	bool m_FramebufferResized = false;

	Camera* GetCamera();

	void InitKeyPressedMap();

	std::map<int, bool>* GetKeyPressedMap();

	void ProcessKeyInput();

private:
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	bool isDeviceSuitable(VkPhysicalDevice device);


	std::vector<Mesh*> m_Meshes;

	VkInstance m_Instance = VK_NULL_HANDLE;
	GlfwWindow m_Window;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VmaAllocator m_Allocator;
	QueueFamilyIndices m_QueueFamilyIndices;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;
	VkQueue m_TranferQueue = VK_NULL_HANDLE;
	SwapChain m_SwapChain;
	RenderPass m_RenderPass;
	VkPipeline m_GraphicPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_GraphicPipelineLayout = VK_NULL_HANDLE;
	VkCommandPool m_TransferPool = VK_NULL_HANDLE;
	VkCommandPool m_GraphicPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	std::vector<VkFence> m_ImagesInFlight;
	Image m_DepthImage;
	Descriptor m_PerMeshDescriptor;
	Descriptor m_PerPassDescriptor;

	Camera* m_Camera;
	MVP m_Mvp;

	std::map<int, bool> m_IsKeyPressedMap;

	uint32_t m_CurrentFrame = 0;

	std::chrono::steady_clock::time_point m_LastTime = std::chrono::high_resolution_clock::now();
	double m_DeltaTime = 0;
};

