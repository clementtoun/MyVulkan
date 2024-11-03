#pragma once

#include "VulkanBase.h"
#include "VulkanUtils.h"
#include "GlfwWindow.h"
#include "QueueVulkan.h"
#include "SwapChain.h"
#include "RenderPass.h"
#include "Shader.h"
#include "Mesh.h"
#include "Materials.h"
#include "Image.h"
#include "Descriptor.h"
#include "Camera.h"
#include "GBuffer.h"
#include "CubeMap.h"
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <map>

constexpr VkClearColorValue clearColor = { { 0.13f, 0.18f, 0.25, 1. } };

const std::string validationLayers = "VK_LAYER_KHRONOS_validation";

const std::vector<std::string> wantedLayers = { "VK_LAYER_LUNARG_monitor" };

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
	VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
	VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
};

#define MAX_FRAMES_IN_FLIGHT 3

typedef struct s_CameraUniform
{
	glm::mat4 view;          
	glm::mat4 projection;    
	glm::vec3 position;      
	float padding;           
} CameraUniform;

typedef struct s_KeyPress
{
	bool current, previous;
} KeyPress;

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

	void CreateGBufferDescriptor();

	void UpdateGBufferDescriptor();

	void CreateCubeMapGraphicPipeline();

	void CreateGraphicPipeline();

	void CreateQuadMesh();

	void CreateCommandPool();

	void CreateCommandBuffers();

	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex, ImDrawData* draw_data);

	void CleanupCommandBuffers() const;

	void CreateSyncObject();

	bool WindowShouldClose();

	void Draw();

	void UpdateUniform();

	bool m_FramebufferResized = false;

	Camera* GetCamera() const;

	void InitKeyPressedMap();

	std::map<int, KeyPress>* GetKeyPressedMap();

	void ProcessKeyInput();

	ImGuiIO* m_io;

	bool gameViewportHovered = false;

private:
	static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	bool isDeviceSuitable(VkPhysicalDevice device);


	std::vector<Mesh*> m_Meshes;
	Materials m_Materials;

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
	RenderPass m_FinalRenderPass;
	VkPipeline m_GraphicPipelineFirstPass = VK_NULL_HANDLE;
	VkPipeline m_GraphicPipelineFirstPassLineMode = VK_NULL_HANDLE;
	VkPipeline m_GraphicPipelineSecondPass = VK_NULL_HANDLE;
	VkPipeline m_GraphicPipelineCubeMap = VK_NULL_HANDLE;
	VkPipelineLayout m_GraphicPipelineFirstPassLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_GraphicPipelineSecondPassLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_GraphicPipelineCubeMapLayout = VK_NULL_HANDLE;
	VkCommandPool m_TransferPool = VK_NULL_HANDLE;
	VkCommandPool m_GraphicPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	Image m_DepthImage;
	Descriptor m_PerMeshDescriptor;
	Descriptor m_PerPassDescriptor;
	Descriptor m_GBufferDescriptor;
	GBuffer m_GBuffer;
	Mesh m_simpleQuadMesh;

	Camera* m_Camera;
	CameraUniform m_CameraUniform;

	std::map<int, KeyPress> m_KeyPressedMap;

	uint32_t m_CurrentFrame = 0;

	std::chrono::steady_clock::time_point m_LastTime = std::chrono::high_resolution_clock::now();
	double m_DeltaTime = 0;

	bool m_Wireframe = false;

	VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

	CubeMap* m_CubeMap;
};

