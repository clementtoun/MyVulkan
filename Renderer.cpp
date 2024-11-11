// ReSharper disable CppZeroConstantCanBeReplacedWithNullptr
// ReSharper disable CppInconsistentNaming
#include "Renderer.h"
#include "MeshLoader.h"
#include <array>

#undef CreateWindow

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->m_FramebufferResized = true;

    auto camera = renderer->GetCamera();

    double aspect = width / static_cast<double>(height);

    if (aspect - std::numeric_limits<double>::epsilon() > 0)
        camera->SetAspect(width / static_cast<double>(height));
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) 
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (renderer->gameViewportHovered || !renderer->m_io->WantCaptureMouse)
    {
        int width, heigth;
        glfwGetFramebufferSize(window, &width, &heigth);

        auto camera = renderer->GetCamera();
        camera->ProcessMouseMouve(xpos / static_cast<double>(width), ypos / static_cast<double>(heigth));
    }

    ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    if (renderer->gameViewportHovered || !renderer->m_io->WantCaptureKeyboard)
    {
        std::map<int, KeyPress>* keyPressMap = renderer->GetKeyPressedMap();

        (*keyPressMap)[key].current = action;
    }

    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    int width, heigth;
    glfwGetFramebufferSize(window, &width, &heigth);

    renderer->GetCamera()->ProcessMouseButton(button, action, mods);

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));

    renderer->GetCamera()->ProcessScroll(xoffset, yoffset);

    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
}

Renderer::Renderer(const std::string& ApplicationName, uint32_t ApplicationVersion, const std::string& EngineName, uint32_t EngineVersion, int width, int height)
{
    InitKeyPressedMap();

    CreateWindow(ApplicationName, width, height);
    glfwSetWindowUserPointer(m_Window.getWindow(), this);
    glfwSetFramebufferSizeCallback(m_Window.getWindow(), framebufferResizeCallback);
    glfwSetCursorPosCallback(m_Window.getWindow(), cursorPosCallback);
    glfwSetKeyCallback(m_Window.getWindow(), keyCallback);
    glfwSetMouseButtonCallback(m_Window.getWindow(), mouseButtonCallback);
    glfwSetScrollCallback(m_Window.getWindow(), scrollCallback);

    CreateInstance(ApplicationName, ApplicationVersion, EngineName, EngineVersion);

    CreateSurface();

    SelectPhysicalDevice();

    CreateLogicalDevice();

    CreateCommandPool();

    CreateVmaAllocator();

    m_SwapChain.BuildSwapChain(m_PhysicalDevice, m_Allocator, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);

    CreateDepthRessources();

    CreateRenderPass();

    auto extent = m_SwapChain.GetExtent();

    m_GBuffer.BuildGBuffer(MAX_FRAMES_IN_FLIGHT, extent.width, extent.height, m_Allocator, m_Device, { m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value()});

    m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_FinalRenderPass.getRenderPass(), m_GBuffer, m_DepthImage.GetImageView());
    
    auto meshes = MeshLoader::loadGltf("./Models/GLTF/DamagedHelmet/glTF/DamagedHelmet.gltf", m_Materials);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->SetModel(glm::translate(glm::mat4(1.f), glm::vec3(20., 0., 0.)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();
    
    meshes = MeshLoader::loadGltf("./Models/GLTF/Cube/glTF/Cube.gltf", m_Materials);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->SetModel(glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(5., 5., 5.)), glm::vec3(0.2f)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();

    auto& cubeMat = m_Materials.GetMaterials()[m_Materials.GetMaterials().size() - 1];
    cubeMat.emissiveTexturePath = "";
    cubeMat.normalTexturePath = "";
    cubeMat.baseColorTexturePath = "";
    cubeMat.metallicRoughnessTexturePath = "";
    cubeMat.materialUniformBuffer.baseColor = glm::vec3(0.);
    cubeMat.materialUniformBuffer.metallic = 0.;
    cubeMat.materialUniformBuffer.roughness = 0.;
    cubeMat.materialUniformBuffer.emissiveColor = glm::vec3(1.f, 0.7f, 0.161f);
    m_PointLights.emplace_back(glm::vec3(5., 5., 5.), glm::vec3(1.f, 0.7f, 0.161f), 1.f);

    std::array<std::string, 6> cubemapFacePaths;
    cubemapFacePaths[0] = "./Textures/skybox/right.jpg";
    cubemapFacePaths[1] = "./Textures/skybox/left.jpg";
    cubemapFacePaths[2] = "./Textures/skybox/top.jpg";
    cubemapFacePaths[3] = "./Textures/skybox/bottom.jpg";
    cubemapFacePaths[4] = "./Textures/skybox/front.jpg";
    cubemapFacePaths[5] = "./Textures/skybox/back.jpg";

    m_CubeMap = new CubeMap(cubemapFacePaths, VK_FORMAT_R8G8B8A8_SRGB, m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_GraphicPool, m_GraphicsQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value(), false);

    m_CubeMap->CreateVertexIndexCubeBuffer(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());

    m_CubeMap->CreateTextureSampler(m_Device);

    meshes = MeshLoader::loadGltf("./Models/GLTF/WaterBottle/glTF/WaterBottle.gltf", m_Materials);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->SetModel(glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(-7., 1., 0.)), glm::vec3(7.)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();
    
    meshes = MeshLoader::loadGltf("./Models/GLTF/Sponza/glTF/Sponza.glb", m_Materials, false, true);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->SetModel(glm::translate(glm::mat4(1.f), glm::vec3(50., 0., 20.)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();
    
    meshes = MeshLoader::loadGltf("./Models/GLTF/MetalRoughSpheres/MetalRoughSpheres.gltf", m_Materials);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->SetModel(glm::translate(mesh->GetModel(), glm::vec3(10., 0., -3.)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();

    meshes = MeshLoader::loadGltf("./Models/GLTF/BoomBox_Axis/BoomBoxWithAxes.gltf", m_Materials);

    for (auto mesh : meshes)
    {
        mesh->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        mesh->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
        //glm::translate(glm::rotate(glm::mat4(1.0), glm::radians(-90.f), glm::vec3(1., 0., 0.)), glm::vec3(35., 0., 20.)));
        mesh->SetModel(glm::scale(mesh->GetModel(), glm::vec3(100.)));
        m_Meshes.push_back(mesh);
    }
    meshes.clear();

    m_DirectionalLights.emplace_back(glm::vec3(-0.5f, -1.f, 0.25f), glm::vec3(1.f), 1.f);

    CreateQuadMesh();

    m_Materials.CreateTexures(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_GraphicPool, m_GraphicsQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());

    m_Materials.CreatePerMaterialDescriptor(m_Allocator, m_Device);
    
    CreatePerMeshDescriptor();

    CreatePerPassDescriptor();

    CreateGBufferDescriptor();

    CreateGraphicPipeline();

    CreateCubeMapGraphicPipeline();

    CreateCommandBuffers();

    CreateSyncObject();

    std::cout << "Graphic index: " << m_QueueFamilyIndices.graphicsFamily.value() << " Compute index: " << m_QueueFamilyIndices.computeFamily.value() << " Transfert index: " << m_QueueFamilyIndices.transferFamily.value() << "\n";

    m_RayTracingAccelerationStructure = new RayTracingAccelerationStructure(m_Device, m_PhysicalDevice, m_Allocator, m_ComputeQueue, m_ComputePool, m_Meshes, m_SwapChain.GetImageViews());

    m_Camera = new QuaternionCamera(glm::vec3(0., 1., 5.), glm::vec3(0., 0., 0.), glm::vec3(0., 1., 0.), 77., extent.width / static_cast<double>(extent.height), 0.5, 1000000.);
    m_Camera->SetSpeed(15.);
    m_Camera->SetMouseSensibility(5.);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = &ImGui::GetIO();
    m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    m_io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 30 },
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 30;
    pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
    pool_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(m_Device, &pool_info, NULL, &m_ImGuiDescriptorPool);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window.getWindow(), false);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Instance;
    init_info.PhysicalDevice = m_PhysicalDevice;
    init_info.Device = m_Device;
    init_info.QueueFamily = m_QueueFamilyIndices.graphicsFamily.value();
    init_info.Queue = m_GraphicsQueue;
    init_info.PipelineCache = NULL;
    init_info.DescriptorPool = m_ImGuiDescriptorPool;
    init_info.RenderPass = m_FinalRenderPass.getRenderPass();
    init_info.Subpass = 0;
    init_info.MinImageCount = static_cast<uint32_t>(m_SwapChain.GetFinalImageViews().size());
    init_info.ImageCount = static_cast<uint32_t>(m_SwapChain.GetFinalImageViews().size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = NULL;
    init_info.CheckVkResultFn = NULL;
    ImGui_ImplVulkan_Init(&init_info);

    m_SwapChain.CreateImGuiImageDescriptor(m_Device);
}

Renderer::~Renderer()
{
    delete m_Camera;

    vkQueueWaitIdle(m_GraphicsQueue);

    if (m_Device != VK_NULL_HANDLE && m_Allocator != VK_NULL_HANDLE && m_RayTracingAccelerationStructure)
    {
        m_RayTracingAccelerationStructure->Cleanup(m_Device, m_Allocator);
        delete m_RayTracingAccelerationStructure;
    }

    if (m_Device != VK_NULL_HANDLE)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }
    }

    CleanupCommandBuffers();

    if (m_CubeMap && m_Device != VK_NULL_HANDLE)
        m_CubeMap->Cleanup(m_Allocator, m_Device);

    if (m_Device != VK_NULL_HANDLE)
    {
        for (auto mesh : m_Meshes)
        {
            mesh->DestroyBuffer(m_Allocator, m_Device);
            delete mesh;
        }

        m_simpleQuadMesh.DestroyBuffer(m_Allocator, m_Device);
    }

    if (m_Device != VK_NULL_HANDLE && m_TransferPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_Device, m_TransferPool, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_Device, m_GraphicPool, NULL);

    if (m_Device != VK_NULL_HANDLE && m_ComputePool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_Device, m_ComputePool, NULL);

    if (m_Device != VK_NULL_HANDLE)
    {
        m_PerMeshDescriptor.DestroyDescriptorPool(m_Device);
        m_PerMeshDescriptor.DestroyDescriptorSetLayout(m_Device);
        m_PerMeshDescriptor.DestroyUniformBuffer(m_Allocator, m_Device);

        m_PerPassDescriptor.DestroyDescriptorPool(m_Device);
        m_PerPassDescriptor.DestroyDescriptorSetLayout(m_Device);
        m_PerPassDescriptor.DestroyUniformBuffer(m_Allocator, m_Device);
        m_PerPassDescriptor.DestroyStorageBuffer(m_Allocator, m_Device);

        m_Materials.Cleanup(m_Allocator, m_Device);

        m_GBufferDescriptor.DestroyDescriptorPool(m_Device);
        m_GBufferDescriptor.DestroyDescriptorSetLayout(m_Device);
        m_GBufferDescriptor.DestroyUniformBuffer(m_Allocator, m_Device);
    }

    if (m_Device != VK_NULL_HANDLE)
        m_GBuffer.Cleanup(m_Allocator, m_Device);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineFirstPass != VK_NULL_HANDLE)
        vkDestroyPipeline(m_Device, m_GraphicPipelineFirstPass, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineFirstPassLineMode != VK_NULL_HANDLE)
        vkDestroyPipeline(m_Device, m_GraphicPipelineFirstPassLineMode, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineFirstPassLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(m_Device, m_GraphicPipelineFirstPassLayout, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineSecondPass != VK_NULL_HANDLE)
        vkDestroyPipeline(m_Device, m_GraphicPipelineSecondPass, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineSecondPassLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(m_Device, m_GraphicPipelineSecondPassLayout, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineCubeMapLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(m_Device, m_GraphicPipelineCubeMapLayout, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineCubeMap != VK_NULL_HANDLE)
        vkDestroyPipeline(m_Device, m_GraphicPipelineCubeMap, NULL);

    if (m_Device != VK_NULL_HANDLE)
        m_RenderPass.cleanup(m_Device);

    if (m_Device != VK_NULL_HANDLE)
        m_FinalRenderPass.cleanup(m_Device);

    if (m_Device != VK_NULL_HANDLE)
        m_DepthImage.Cleanup(m_Allocator, m_Device);

    if (m_Device != VK_NULL_HANDLE)
        m_SwapChain.Cleanup(m_Device, m_Allocator);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_Device)
        vkDestroyDescriptorPool(m_Device, m_ImGuiDescriptorPool, NULL);

    vmaDestroyAllocator(m_Allocator);

    if (m_Device != VK_NULL_HANDLE)
        vkDestroyDevice(m_Device, NULL);

    if (m_Instance != VK_NULL_HANDLE && m_Surface != VK_NULL_HANDLE)
        vkDestroySurfaceKHR(m_Instance, m_Surface, NULL);

    if (m_Instance != VK_NULL_HANDLE)
        vkDestroyInstance(m_Instance, NULL);

    m_Window.destroyWindow();
    m_Window.terminateGlfw();
}

void Renderer::CreateWindow(const std::string& WindowName, int width, int height)
{
    if (!m_Window.createWindow(WindowName, width, height))
        std::cout << "GLFW Window creation failed !" << '\n';
}

void Renderer::CreateInstance(const std::string& ApplicationName, uint32_t ApplicationVersion, const std::string& EngineName, uint32_t EngineVersion)
{
    VkApplicationInfo vkApplicationInfo;
    vkApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vkApplicationInfo.pNext = NULL;
    vkApplicationInfo.pApplicationName = ApplicationName.c_str();
    vkApplicationInfo.applicationVersion = ApplicationVersion;
    vkApplicationInfo.pEngineName = EngineName.c_str();
    vkApplicationInfo.engineVersion = EngineVersion;
    vkApplicationInfo.apiVersion = VK_API_VERSION_1_3;

    uint32_t extensionsCount;
    auto extensionsNames = m_Window.getRequiredInstanceExtesions(extensionsCount);

    uint32_t propertyCount;
    vkEnumerateInstanceLayerProperties(&propertyCount, NULL);

    std::vector<VkLayerProperties> layersProperties(propertyCount);
    vkEnumerateInstanceLayerProperties(&propertyCount, layersProperties.data());

    std::vector<const char*> enabledLayers;

    for (const auto& property : layersProperties)
    {
        for (const auto& wantedLayerName : wantedLayers)
        {
            if (strcmp(wantedLayerName.data(), property.layerName) == 0)
            {
                enabledLayers.push_back(wantedLayerName.data());
            }
        }
    }

    if (enableValidationLayers)
        enabledLayers.push_back(validationLayers.data());

    VkInstanceCreateInfo vkInstanceInfo;
    vkInstanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vkInstanceInfo.pNext = NULL;
    vkInstanceInfo.flags = NULL;
    vkInstanceInfo.pApplicationInfo = &vkApplicationInfo;
    vkInstanceInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    vkInstanceInfo.ppEnabledLayerNames = enabledLayers.data();
    vkInstanceInfo.enabledExtensionCount = extensionsCount;
    vkInstanceInfo.ppEnabledExtensionNames = extensionsNames;

    if (vkCreateInstance(&vkInstanceInfo, NULL, &m_Instance) != VK_SUCCESS)
        std::cout << "Vulkan instance creation failed !" << '\n';
}

void Renderer::CreateSurface()
{
    if (m_Window.createSurface(m_Instance, m_Surface) != VK_SUCCESS)
        std::cout << "Vulkan surface creation failed !" << '\n';
}

void Renderer::SelectPhysicalDevice()
{
    uint32_t physicalDeviceCount;

    if (vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
    {
        std::cout << "Fail to get number of device found !" << '\n';
    }
    else if (physicalDeviceCount <= 0)
    {
        std::cout << "No physical device found !" << '\n';
    }

    std::vector<VkPhysicalDevice> vkPysicalDevices(physicalDeviceCount);

    if (vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, vkPysicalDevices.data()) != VK_SUCCESS)
    {
        std::cout << "Failed to get physical device !" << '\n';
    }

    for (const auto& pysicalDevice : vkPysicalDevices) {
        if (isDeviceSuitable(pysicalDevice)) {
            m_PhysicalDevice = pysicalDevice;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        std::cout << "No GPU can run this program!" << '\n';
    }
}

void Renderer::CreateVmaAllocator()
{
    /*
    VmaVulkanFunctions vulkanFunctions;
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    vulkanFunctions.vkGetPhysicalDeviceProperties = &vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = &vkGetPhysicalDeviceMemoryProperties; 
    vulkanFunctions.vkAllocateMemory = &vkAllocateMemory; 
    vulkanFunctions.vkFreeMemory = &vkFreeMemory; 
    vulkanFunctions.vkMapMemory = &vkMapMemory; 
    vulkanFunctions.vkUnmapMemory = &vkUnmapMemory; 
    vulkanFunctions.vkFlushMappedMemoryRanges = &vkFlushMappedMemoryRanges; 
    vulkanFunctions.vkInvalidateMappedMemoryRanges = &vkInvalidateMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory = &vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory = &vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements = &vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements = &vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer = &vkCreateBuffer;
    vulkanFunctions.vkDestroyBuffer = &vkDestroyBuffer;
    vulkanFunctions.vkCreateImage = &vkCreateImage;
    vulkanFunctions.vkDestroyImage = &vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer = &vkCmdCopyBuffer;
    vulkanFunctions.vkGetBufferMemoryRequirements2KHR = &vkGetBufferMemoryRequirements2;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR = &vkGetImageMemoryRequirements2;
    vulkanFunctions.vkBindBufferMemory2KHR = &vkBindBufferMemory2;
    vulkanFunctions.vkBindImageMemory2KHR = &vkBindImageMemory2;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = &vkGetPhysicalDeviceMemoryProperties2;
    vulkanFunctions.vkGetDeviceBufferMemoryRequirements = &vkGetDeviceBufferMemoryRequirements;
    vulkanFunctions.vkGetDeviceImageMemoryRequirements = &vkGetDeviceImageMemoryRequirements;
    */

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_Device;
    allocatorCreateInfo.instance = m_Instance;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    //allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    if (vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator) != VK_SUCCESS)
        std::cout << "Vma allocator creation failed !" << '\n';
}

void Renderer::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices queueFamilyIndices;

    // Code pour trouver les indices de familles � ajouter � la structure
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndices.graphicsFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueFamilyIndices.computeFamily = i;
        }

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            queueFamilyIndices.transferFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            queueFamilyIndices.presentFamily = i;
        }

        if (queueFamilyIndices.isComplete()) {
            break;
        }

        i++;
    }

    m_QueueFamilyIndices = queueFamilyIndices;
}

void Renderer::CreateLogicalDevice()
{
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value(), m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.computeFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo vkDeviceQueueCreateInfo;
        vkDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        vkDeviceQueueCreateInfo.pNext = NULL;
        vkDeviceQueueCreateInfo.flags = 0;
        vkDeviceQueueCreateInfo.queueCount = 1;
        vkDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
        vkDeviceQueueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(vkDeviceQueueCreateInfo);
    }

    std::vector<const char*> enabledLayers;

    if (enableValidationLayers)
        enabledLayers.push_back(validationLayers.data());

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    deviceFeatures.features.sampleRateShading = VK_FALSE; // enable sample shading feature for the device

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;  // Enable ray tracing pipeline
    
    // Enable buffer device address feature as well
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

    // Link the feature structures in the pNext chain
    deviceFeatures.pNext = &bufferDeviceAddressFeatures;
    bufferDeviceAddressFeatures.pNext = &rayTracingPipelineFeatures;
    rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;
    
    VkDeviceCreateInfo vkDeviceCreateInfo{};
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.flags = 0;
    vkDeviceCreateInfo.pNext = &deviceFeatures;
    vkDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    vkDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    vkDeviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    vkDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(m_PhysicalDevice, &vkDeviceCreateInfo, NULL, &m_Device) != VK_SUCCESS)
    {
        std::cout << "Logical device creation failed !" << '\n';
    }

    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.transferFamily.value(), 0, &m_TranferQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.computeFamily.value(), 0, &m_ComputeQueue);

    if (m_GraphicsQueue == VK_NULL_HANDLE)
    {
        std::cout << "Graphics queue retrieve failed !" << '\n';
    }
    if (m_PresentQueue == VK_NULL_HANDLE)
    {
        std::cout << "Present queue retrieve failed !" << '\n';
    }
    if (m_TranferQueue == VK_NULL_HANDLE)
    {
        std::cout << "Tranfer queue retrieve failed !" << '\n';
    }

    if (m_ComputeQueue == VK_NULL_HANDLE)
    {
        std::cout << "Compute queue retrieve failed !" << '\n';
    }
}

void Renderer::CreateDepthRessources()
{
    VkFormat depthStencilFormat = Image::findDepthFormat(m_PhysicalDevice);

    m_DepthImage.CreateImage(m_Allocator, m_SwapChain.GetExtent().width, m_SwapChain.GetExtent().height,
        depthStencilFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { m_QueueFamilyIndices.graphicsFamily.value() });
    m_DepthImage.CreateImageView(m_Device, depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::CreateRenderPass()
{
    VkAttachmentDescription posAttachmentDescription;
    posAttachmentDescription.flags = 0;
    posAttachmentDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    posAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    posAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    posAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    posAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    posAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    posAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    posAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription normalAttachmentDescription;
    normalAttachmentDescription.flags = 0;
    normalAttachmentDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    normalAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    normalAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    normalAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    normalAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    normalAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    normalAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentDescription;
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription pbrAttachmentDescription;
    pbrAttachmentDescription.flags = 0;
    pbrAttachmentDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
    pbrAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    pbrAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    pbrAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pbrAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    pbrAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    pbrAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    pbrAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription emissiveAttachmentDescription;
    emissiveAttachmentDescription.flags = 0;
    emissiveAttachmentDescription.format = VK_FORMAT_R8G8B8A8_UNORM;
    emissiveAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    emissiveAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    emissiveAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    emissiveAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    emissiveAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    emissiveAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    emissiveAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachmentDescription;
    depthAttachmentDescription.flags = 0;
    depthAttachmentDescription.format = Image::findDepthFormat(m_PhysicalDevice);
    depthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference posGBufferReference{};
    posGBufferReference.attachment = 0;
    posGBufferReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference normalGBufferReference{};
    normalGBufferReference.attachment = 1;
    normalGBufferReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorGBufferReference{};
    colorGBufferReference.attachment = 2;
    colorGBufferReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference pbrGBufferReference{};
    pbrGBufferReference.attachment = 3;
    pbrGBufferReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference emissiveGBufferReference{};
    emissiveGBufferReference.attachment = 4;
    emissiveGBufferReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 5;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachmentDescription;
    attachmentDescription.flags = 0;
    attachmentDescription.format = m_SwapChain.GetSurfaceFormat();
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 6;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_RenderPass.addSubPass(VK_PIPELINE_BIND_POINT_GRAPHICS, {}, { posGBufferReference, normalGBufferReference, colorGBufferReference, pbrGBufferReference, emissiveGBufferReference }, {}, { depthAttachmentRef }, { } );

    m_RenderPass.addSubPassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

    VkAttachmentReference inPosGBufferReference{};
    inPosGBufferReference.attachment = 0;
    inPosGBufferReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inNormalGBufferReference{};
    inNormalGBufferReference.attachment = 1;
    inNormalGBufferReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inColorGBufferReference{};
    inColorGBufferReference.attachment = 2;
    inColorGBufferReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inPbrGBufferReference{};
    inPbrGBufferReference.attachment = 3;
    inPbrGBufferReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference inEmissiveGBufferReference{};
    inEmissiveGBufferReference.attachment = 4;
    inEmissiveGBufferReference.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    m_RenderPass.addSubPass(VK_PIPELINE_BIND_POINT_GRAPHICS, { inPosGBufferReference, inNormalGBufferReference, inColorGBufferReference, inPbrGBufferReference, inEmissiveGBufferReference }, { attachmentReference }, {}, {}, {});

    m_RenderPass.addSubPassDependency(0, 1, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_DEPENDENCY_BY_REGION_BIT);

    m_RenderPass.addSubPassDependency(1, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_DEPENDENCY_BY_REGION_BIT);

    m_RenderPass.CreateRenderPass(m_Device, { posAttachmentDescription, normalAttachmentDescription, colorAttachmentDescription, pbrAttachmentDescription, emissiveAttachmentDescription, depthAttachmentDescription, attachmentDescription });

    VkAttachmentDescription finalAttachmentDescription;
    finalAttachmentDescription.flags = 0;
    finalAttachmentDescription.format = m_SwapChain.GetSurfaceFormat();
    finalAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    finalAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    finalAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    finalAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    finalAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    finalAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    finalAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference finalAttachmentReference;
    finalAttachmentReference.attachment = 0;
    finalAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    m_FinalRenderPass.addSubPass(VK_PIPELINE_BIND_POINT_GRAPHICS, {}, { finalAttachmentReference }, {}, {}, {});

    m_FinalRenderPass.addSubPassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, 0);

    m_FinalRenderPass.addSubPassDependency(0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

    m_FinalRenderPass.CreateRenderPass(m_Device, { finalAttachmentDescription });
}

void Renderer::CreatePerMeshDescriptor()
{
    uint32_t numMesh = static_cast<uint32_t>(m_Meshes.size());

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = NULL;

    m_PerMeshDescriptor.CreateDescriptorSetLayout(m_Device, { descriptorSetLayoutBinding }, 0);

    if (numMesh > 0)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

        uint64_t bufferSize = sizeof(glm::mat4);
        uint64_t paddedSize = (bufferSize + minSize - 1) & ~(minSize - 1);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            m_PerMeshDescriptor.AddUniformBuffer(m_Allocator, m_Meshes.size() * paddedSize);
        }

        VkDescriptorPoolSize descriptorPoolSize;
        descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        m_PerMeshDescriptor.CreateDescriptorPool(m_Device, { descriptorPoolSize }, MAX_FRAMES_IN_FLIGHT);

        VkDescriptorSetLayout layout = m_PerMeshDescriptor.GetDescriptorSetLayout();
        std::vector<VkDescriptorSetLayout> layouts;
        layouts.assign(MAX_FRAMES_IN_FLIGHT, layout);

        m_PerMeshDescriptor.AllocateDescriptorSet(m_Device, layouts);

        auto uniformBuffers = m_PerMeshDescriptor.GetUniformBuffers();
        auto descriptorSets = m_PerMeshDescriptor.GetDescriptorSets();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            for (size_t j = 0; j < m_Meshes.size(); j++)
            {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[i];
                bufferInfo.offset = 0;
                bufferInfo.range = paddedSize;

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = descriptorSets[i];
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfo;
                descriptorWrite.pImageInfo = nullptr; // Optional
                descriptorWrite.pTexelBufferView = nullptr; // Optional

                vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);
            }
        }
    }
}

void Renderer::CreateGBufferDescriptor()
{
    std::vector<VkDescriptorSetLayoutBinding> attachmentLayoutBinding;
    attachmentLayoutBinding.resize(5);

    attachmentLayoutBinding[0].binding = 0;
    attachmentLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentLayoutBinding[0].descriptorCount = 1;
    attachmentLayoutBinding[0].pImmutableSamplers = NULL;
    attachmentLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    attachmentLayoutBinding[1].binding = 1;
    attachmentLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentLayoutBinding[1].descriptorCount = 1;
    attachmentLayoutBinding[1].pImmutableSamplers = NULL;
    attachmentLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    attachmentLayoutBinding[2].binding = 2;
    attachmentLayoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentLayoutBinding[2].descriptorCount = 1;
    attachmentLayoutBinding[2].pImmutableSamplers = NULL;
    attachmentLayoutBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    attachmentLayoutBinding[3].binding = 3;
    attachmentLayoutBinding[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentLayoutBinding[3].descriptorCount = 1;
    attachmentLayoutBinding[3].pImmutableSamplers = NULL;
    attachmentLayoutBinding[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    attachmentLayoutBinding[4].binding = 4;
    attachmentLayoutBinding[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentLayoutBinding[4].descriptorCount = 1;
    attachmentLayoutBinding[4].pImmutableSamplers = NULL;
    attachmentLayoutBinding[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    auto GBufferImages = m_GBuffer.GetGBufferImages();

    m_GBufferDescriptor.CreateDescriptorSetLayout(m_Device, attachmentLayoutBinding, 0);

    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    poolSize.descriptorCount = static_cast<uint32_t>(GBufferImages.size());

    m_GBufferDescriptor.CreateDescriptorPool(m_Device, { poolSize }, static_cast<uint32_t>(GBufferImages.size()));
    
    std::vector<VkDescriptorSetLayout> layouts; 
    layouts.assign(GBufferImages.size(), m_GBufferDescriptor.GetDescriptorSetLayout());

    m_GBufferDescriptor.AllocateDescriptorSet(m_Device, layouts);

    UpdateGBufferDescriptor();
}

void Renderer::UpdateGBufferDescriptor()
{
    auto GBufferImages = m_GBuffer.GetGBufferImages();
    auto descriptorSets = m_GBufferDescriptor.GetDescriptorSets();

    for (size_t i = 0; i < GBufferImages.size(); i++)
    {
        VkDescriptorImageInfo posDescriptor{};
        posDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        posDescriptor.imageView = GBufferImages[i].positionImageBuffer.GetImageView();
        posDescriptor.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo normalDescriptor{};
        normalDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalDescriptor.imageView = GBufferImages[i].normalImageBuffer.GetImageView();
        normalDescriptor.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo colorDescriptor{};
        colorDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorDescriptor.imageView = GBufferImages[i].colorImageBuffer.GetImageView();
        colorDescriptor.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo pbrDescriptor{};
        pbrDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        pbrDescriptor.imageView = GBufferImages[i].pbrImageBuffer.GetImageView();
        pbrDescriptor.sampler = VK_NULL_HANDLE;

        VkDescriptorImageInfo emissiveDescriptor{};
        emissiveDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emissiveDescriptor.imageView = GBufferImages[i].emissiveImageBuffer.GetImageView();
        emissiveDescriptor.sampler = VK_NULL_HANDLE;

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].pImageInfo = &posDescriptor;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].pImageInfo = &normalDescriptor;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].pImageInfo = &colorDescriptor;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].pImageInfo = &pbrDescriptor;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = descriptorSets[i];
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].pImageInfo = &emissiveDescriptor;

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::CreateCubeMapGraphicPipeline()
{
    Shader firstVertexShader;
    firstVertexShader.createModule(m_Device, ".\\Shader\\cubeMapVert.spv");
    Shader firstFragmentShader;
    firstFragmentShader.createModule(m_Device, ".\\Shader\\cubeMapFrag.spv");

    VkPipelineShaderStageCreateInfo pipelineFirstVertexShaderStageCreateInfo;
    pipelineFirstVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFirstVertexShaderStageCreateInfo.pNext = NULL;
    pipelineFirstVertexShaderStageCreateInfo.flags = 0;
    pipelineFirstVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineFirstVertexShaderStageCreateInfo.module = firstVertexShader.getShaderModule();
    pipelineFirstVertexShaderStageCreateInfo.pName = "main";
    pipelineFirstVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo pipelineFirstFragmentShaderStageCreateInfo;
    pipelineFirstFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFirstFragmentShaderStageCreateInfo.pNext = NULL;
    pipelineFirstFragmentShaderStageCreateInfo.flags = 0;
    pipelineFirstFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineFirstFragmentShaderStageCreateInfo.module = firstFragmentShader.getShaderModule();
    pipelineFirstFragmentShaderStageCreateInfo.pName = "main";
    pipelineFirstFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

    std::array<VkPipelineShaderStageCreateInfo, 2> firstPipelineShaderStageCreateInfos = { pipelineFirstVertexShaderStageCreateInfo, pipelineFirstFragmentShaderStageCreateInfo };

    auto vertexInputBindingDescription = VertexPos::getVertexInputBindingDescription();
    auto vertexInputAttributeDescription = VertexPos::getVertexInputAttributeDescription();

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = NULL;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescription.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.pNext = NULL;
    pipelineInputAssemblyStateCreateInfo.flags = 0;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo;
    pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pipelineTessellationStateCreateInfo.pNext = NULL;
    pipelineTessellationStateCreateInfo.flags = 0;
    pipelineTessellationStateCreateInfo.patchControlPoints = 3;

    auto extent = m_SwapChain.GetExtent();

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.pNext = NULL;
    pipelineViewportStateCreateInfo.flags = 0;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.pViewports = &viewport;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.pNext = NULL;
    pipelineRasterizationStateCreateInfo.flags = 0;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0;
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0;
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.pNext = NULL;
    pipelineMultisampleStateCreateInfo.flags = 0;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // Optionnel
    pipelineMultisampleStateCreateInfo.pSampleMask = nullptr; // Optionnel
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optionnel
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optionnel

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.pNext = NULL;
    pipelineDepthStencilStateCreateInfo.flags = 0;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0;
    pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.front = {};
    pipelineDepthStencilStateCreateInfo.back = {};

    VkPipelineColorBlendAttachmentState pPipelineColorBlendAttachmentState;
    pPipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pPipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pPipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pPipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pPipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pPipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pPipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    pPipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    std::array<VkPipelineColorBlendAttachmentState, 5> pPipelineColorBlendAttachmentStates = { pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState };
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.pNext = NULL;
    pipelineColorBlendStateCreateInfo.flags = 0;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    pipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(pPipelineColorBlendAttachmentStates.size());
    pipelineColorBlendStateCreateInfo.pAttachments = pPipelineColorBlendAttachmentStates.data();
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = NULL;
    pipelineDynamicStateCreateInfo.flags = 0;
    pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    
    std::array<VkDescriptorSetLayout, 1> firstPassLayouts = { m_PerPassDescriptor.GetDescriptorSetLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutFirstPassCreateInfo;
    pipelineLayoutFirstPassCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutFirstPassCreateInfo.pNext = NULL;
    pipelineLayoutFirstPassCreateInfo.flags = 0;
    pipelineLayoutFirstPassCreateInfo.setLayoutCount = static_cast<uint32_t>(firstPassLayouts.size());
    pipelineLayoutFirstPassCreateInfo.pSetLayouts = firstPassLayouts.data();
    pipelineLayoutFirstPassCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutFirstPassCreateInfo.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(m_Device, &pipelineLayoutFirstPassCreateInfo, NULL, &m_GraphicPipelineCubeMapLayout) != VK_SUCCESS)
        std::cout << "Pipeline layout creation failed !" << '\n';

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = NULL;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(firstPipelineShaderStageCreateInfos.size());
    graphicsPipelineCreateInfo.pStages = firstPipelineShaderStageCreateInfos.data();
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = m_GraphicPipelineCubeMapLayout;
    graphicsPipelineCreateInfo.renderPass = m_RenderPass.getRenderPass();
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &m_GraphicPipelineCubeMap) != VK_SUCCESS)
        std::cout << "Pipeline cration failed !" << '\n';

    firstVertexShader.cleanup(m_Device);
    firstFragmentShader.cleanup(m_Device);
}

void Renderer::CreatePerPassDescriptor()
{
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding;
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding descriptorSetCubeMapLayoutBinding{};
    descriptorSetCubeMapLayoutBinding.binding = 1;
    descriptorSetCubeMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetCubeMapLayoutBinding.descriptorCount = 1;
    descriptorSetCubeMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorSetCubeMapLayoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding descriptorDirectionalLightsLayoutBinding{};
    descriptorDirectionalLightsLayoutBinding.binding = 2;
    descriptorDirectionalLightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorDirectionalLightsLayoutBinding.descriptorCount = 1;
    descriptorDirectionalLightsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorDirectionalLightsLayoutBinding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutBinding descriptorPointLightsLayoutBinding{};
    descriptorPointLightsLayoutBinding.binding = 3;
    descriptorPointLightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorPointLightsLayoutBinding.descriptorCount = 1;
    descriptorPointLightsLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptorPointLightsLayoutBinding.pImmutableSamplers = NULL;

    m_PerPassDescriptor.CreateDescriptorSetLayout(m_Device, { descriptorSetLayoutBinding, descriptorSetCubeMapLayoutBinding, descriptorDirectionalLightsLayoutBinding, descriptorPointLightsLayoutBinding }, 0);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_PerPassDescriptor.AddUniformBuffer(m_Allocator, sizeof(SceneUniform));
        m_PerPassDescriptor.AddStorageBuffer(m_Allocator, m_DirectionalLights.size() * sizeof(UniformDirectionalLight));
        m_PerPassDescriptor.AddStorageBuffer(m_Allocator, m_PointLights.size() * sizeof(UniformPointLight));
    }

    std::vector<VkDescriptorPoolSize> poolSizes{};
    VkDescriptorPoolSize poolSize;

    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes.push_back(poolSize);

    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes.push_back(poolSize);

    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
    poolSizes.push_back(poolSize);

    m_PerPassDescriptor.CreateDescriptorPool(m_Device, poolSizes, MAX_FRAMES_IN_FLIGHT);

    VkDescriptorSetLayout layout = m_PerPassDescriptor.GetDescriptorSetLayout();
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.assign(MAX_FRAMES_IN_FLIGHT, layout);

    m_PerPassDescriptor.AllocateDescriptorSet(m_Device, layouts);

    auto uniformBuffers = m_PerPassDescriptor.GetUniformBuffers();
    auto storageBuffers = m_PerPassDescriptor.GetUniformStorageBuffers();
    auto descriptorSets = m_PerPassDescriptor.GetDescriptorSets();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(SceneUniform);

        std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        VkDescriptorImageInfo cubeMapInfo{};
        cubeMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cubeMapInfo.imageView = m_CubeMap->GetImageView();
        cubeMapInfo.sampler = m_CubeMap->GetTextureSampler();

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &cubeMapInfo;

        VkDescriptorBufferInfo directionalLightBufferInfo{};
        directionalLightBufferInfo.buffer = storageBuffers[i*2].buffer;
        directionalLightBufferInfo.offset = 0;
        directionalLightBufferInfo.range = m_DirectionalLights.size() * sizeof(UniformDirectionalLight);

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &directionalLightBufferInfo;

        VkDescriptorBufferInfo pointLightBufferInfo{};
        pointLightBufferInfo.buffer = storageBuffers[i*2+1].buffer;
        pointLightBufferInfo.offset = 0;
        pointLightBufferInfo.range = m_PointLights.size() * sizeof(UniformPointLight);

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = descriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &pointLightBufferInfo;

        vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void Renderer::CreateGraphicPipeline()
{
    Shader firstVertexShader;
    firstVertexShader.createModule(m_Device, ".\\Shader\\firstVert.spv");
    Shader firstFragmentShader;
    firstFragmentShader.createModule(m_Device, ".\\Shader\\firstFrag.spv");

    Shader secondVertexShader;
    secondVertexShader.createModule(m_Device, ".\\Shader\\secondVert.spv");
    Shader secondFragmentShader;
    secondFragmentShader.createModule(m_Device, ".\\Shader\\secondFrag.spv");

    VkPipelineShaderStageCreateInfo pipelineFirstVertexShaderStageCreateInfo;
    pipelineFirstVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFirstVertexShaderStageCreateInfo.pNext = NULL;
    pipelineFirstVertexShaderStageCreateInfo.flags = 0;
    pipelineFirstVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineFirstVertexShaderStageCreateInfo.module = firstVertexShader.getShaderModule();
    pipelineFirstVertexShaderStageCreateInfo.pName = "main";
    pipelineFirstVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo pipelineFirstFragmentShaderStageCreateInfo;
    pipelineFirstFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFirstFragmentShaderStageCreateInfo.pNext = NULL;
    pipelineFirstFragmentShaderStageCreateInfo.flags = 0;
    pipelineFirstFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineFirstFragmentShaderStageCreateInfo.module = firstFragmentShader.getShaderModule();
    pipelineFirstFragmentShaderStageCreateInfo.pName = "main";
    pipelineFirstFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

    std::array<VkPipelineShaderStageCreateInfo, 2> firstPipelineShaderStageCreateInfos = { pipelineFirstVertexShaderStageCreateInfo, pipelineFirstFragmentShaderStageCreateInfo };

    VkPipelineShaderStageCreateInfo pipelineSecondVertexShaderStageCreateInfo;
    pipelineSecondVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineSecondVertexShaderStageCreateInfo.pNext = NULL;
    pipelineSecondVertexShaderStageCreateInfo.flags = 0;
    pipelineSecondVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineSecondVertexShaderStageCreateInfo.module = secondVertexShader.getShaderModule();
    pipelineSecondVertexShaderStageCreateInfo.pName = "main";
    pipelineSecondVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo pipelineSecondFragmentShaderStageCreateInfo;
    pipelineSecondFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineSecondFragmentShaderStageCreateInfo.pNext = NULL;
    pipelineSecondFragmentShaderStageCreateInfo.flags = 0;
    pipelineSecondFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineSecondFragmentShaderStageCreateInfo.module = secondFragmentShader.getShaderModule();
    pipelineSecondFragmentShaderStageCreateInfo.pName = "main";
    pipelineSecondFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

    std::array<VkPipelineShaderStageCreateInfo, 2> secondPipelineShaderStageCreateInfos = { pipelineSecondVertexShaderStageCreateInfo, pipelineSecondFragmentShaderStageCreateInfo };

    auto vertexInputBindingDescription = Vertex::getVertexInputBindingDescription();
    auto vertexInputAttributeDescription = Vertex::getVertexInputAttributeDescription();

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.pNext = NULL;
    pipelineVertexInputStateCreateInfo.flags = 0;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescription.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo;
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.pNext = NULL;
    pipelineInputAssemblyStateCreateInfo.flags = 0;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo;
    pipelineTessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pipelineTessellationStateCreateInfo.pNext = NULL;
    pipelineTessellationStateCreateInfo.flags = 0;
    pipelineTessellationStateCreateInfo.patchControlPoints = 3;

    auto extent = m_SwapChain.GetExtent();

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo;
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.pNext = NULL;
    pipelineViewportStateCreateInfo.flags = 0;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.pViewports = &viewport;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo;
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.pNext = NULL;
    pipelineRasterizationStateCreateInfo.flags = 0;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0;
    pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0;
    pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0;

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo;
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.pNext = NULL;
    pipelineMultisampleStateCreateInfo.flags = 0;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.minSampleShading = 1.0f; // Optionnel
    pipelineMultisampleStateCreateInfo.pSampleMask = nullptr; // Optionnel
    pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optionnel
    pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE; // Optionnel

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo;
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.pNext = NULL;
    pipelineDepthStencilStateCreateInfo.flags = 0;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0;
    pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.front = {};
    pipelineDepthStencilStateCreateInfo.back = {};

    VkPipelineColorBlendAttachmentState pPipelineColorBlendAttachmentState;
    pPipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
    pPipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pPipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    pPipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pPipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pPipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    pPipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
    pPipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    std::array<VkPipelineColorBlendAttachmentState, 5> pPipelineColorBlendAttachmentStates = { pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState,
                                                                                               pPipelineColorBlendAttachmentState };
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.pNext = NULL;
    pipelineColorBlendStateCreateInfo.flags = 0;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    pipelineColorBlendStateCreateInfo.attachmentCount = static_cast<uint32_t>(pPipelineColorBlendAttachmentStates.size());
    pipelineColorBlendStateCreateInfo.pAttachments = pPipelineColorBlendAttachmentStates.data();
    pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f;
    pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

    std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo;
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.pNext = NULL;
    pipelineDynamicStateCreateInfo.flags = 0;
    pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
    
    std::array<VkDescriptorSetLayout,3> firstPassLayouts = {m_PerPassDescriptor.GetDescriptorSetLayout(), m_PerMeshDescriptor.GetDescriptorSetLayout(), m_Materials.GetDescriptorSetsLayout()};

    VkPipelineLayoutCreateInfo pipelineLayoutFirstPassCreateInfo;
    pipelineLayoutFirstPassCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutFirstPassCreateInfo.pNext = NULL;
    pipelineLayoutFirstPassCreateInfo.flags = 0;
    pipelineLayoutFirstPassCreateInfo.setLayoutCount = static_cast<uint32_t>(firstPassLayouts.size());
    pipelineLayoutFirstPassCreateInfo.pSetLayouts = firstPassLayouts.data();
    pipelineLayoutFirstPassCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutFirstPassCreateInfo.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(m_Device, &pipelineLayoutFirstPassCreateInfo, NULL, &m_GraphicPipelineFirstPassLayout) != VK_SUCCESS)
        std::cout << "Pipeline layout creation failed !" << '\n';

    VkGraphicsPipelineCreateInfo graphicsPipelineFirstPassCreateInfo;
    graphicsPipelineFirstPassCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineFirstPassCreateInfo.pNext = NULL;
    graphicsPipelineFirstPassCreateInfo.flags = 0;
    graphicsPipelineFirstPassCreateInfo.stageCount = static_cast<uint32_t>(firstPipelineShaderStageCreateInfos.size());
    graphicsPipelineFirstPassCreateInfo.pStages = firstPipelineShaderStageCreateInfos.data();
    graphicsPipelineFirstPassCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineFirstPassCreateInfo.layout = m_GraphicPipelineFirstPassLayout;
    graphicsPipelineFirstPassCreateInfo.renderPass = m_RenderPass.getRenderPass();
    graphicsPipelineFirstPassCreateInfo.subpass = 0;
    graphicsPipelineFirstPassCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineFirstPassCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineFirstPassCreateInfo, NULL, &m_GraphicPipelineFirstPass) != VK_SUCCESS)
        std::cout << "Pipeline cration failed !" << '\n';

    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_LINE;

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineFirstPassCreateInfo, NULL, &m_GraphicPipelineFirstPassLineMode) != VK_SUCCESS)
        std::cout << "Pipeline cration failed !" << '\n';

    std::array<VkDescriptorSetLayout, 2> secondPassLayouts = { m_PerPassDescriptor.GetDescriptorSetLayout(), m_GBufferDescriptor.GetDescriptorSetLayout() };

    VkPipelineLayoutCreateInfo pipelineLayoutSecondPassCreateInfo;
    pipelineLayoutSecondPassCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutSecondPassCreateInfo.pNext = NULL;
    pipelineLayoutSecondPassCreateInfo.flags = 0;
    pipelineLayoutSecondPassCreateInfo.setLayoutCount = static_cast<uint32_t>(secondPassLayouts.size());
    pipelineLayoutSecondPassCreateInfo.pSetLayouts = secondPassLayouts.data();
    pipelineLayoutSecondPassCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutSecondPassCreateInfo.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(m_Device, &pipelineLayoutSecondPassCreateInfo, NULL, &m_GraphicPipelineSecondPassLayout) != VK_SUCCESS)
        std::cout << "Pipeline layout creation failed !" << '\n';

    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;

    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pPipelineColorBlendAttachmentState;

    pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo graphicsPipelineSecondPassCreateInfo;
    graphicsPipelineSecondPassCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineSecondPassCreateInfo.pNext = NULL;
    graphicsPipelineSecondPassCreateInfo.flags = 0;
    graphicsPipelineSecondPassCreateInfo.stageCount = static_cast<uint32_t>(secondPipelineShaderStageCreateInfos.size());
    graphicsPipelineSecondPassCreateInfo.pStages = secondPipelineShaderStageCreateInfos.data();
    graphicsPipelineSecondPassCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineSecondPassCreateInfo.layout = m_GraphicPipelineSecondPassLayout;
    graphicsPipelineSecondPassCreateInfo.renderPass = m_RenderPass.getRenderPass();
    graphicsPipelineSecondPassCreateInfo.subpass = 1;
    graphicsPipelineSecondPassCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineSecondPassCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineSecondPassCreateInfo, NULL, &m_GraphicPipelineSecondPass) != VK_SUCCESS)
        std::cout << "Pipeline cration failed !" << '\n';

    firstVertexShader.cleanup(m_Device);
    firstFragmentShader.cleanup(m_Device);
    secondVertexShader.cleanup(m_Device);
    secondFragmentShader.cleanup(m_Device);
}

void Renderer::CreateQuadMesh()
{
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(-1., -1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(-1, -1), {0, 0, 0}));
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(1., -1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(1., -1.), {0, 0, 0}));
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(-1., 1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(-1., 1.), {0, 0, 0}));
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(-1., 1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(-1., 1.), {0, 0, 0}));
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(1., -1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(1., -1.), {0, 0, 0}));
    m_simpleQuadMesh.AddVertex(Vertex(glm::vec3(1., 1., 0.), glm::vec3(0.), glm::vec3(0.), glm::vec3(0.), glm::vec2(1., 1.), {0, 0, 0}));

    m_simpleQuadMesh.CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
}

void Renderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicPool) != VK_SUCCESS) {
        std::cout << "Graphic command pool creation failed !" << '\n';
    }

    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.transferFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; 

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_TransferPool) != VK_SUCCESS) {
        std::cout << "Transfer command pool creation failed" << '\n';
    }

    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.computeFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_ComputePool) != VK_SUCCESS) {
        std::cout << "Compute command pool creation failed" << '\n';
    }
}

void Renderer::CreateCommandBuffers() 
{
   m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

   VkCommandBufferAllocateInfo commandBufferAllocateInfo;
   commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   commandBufferAllocateInfo.pNext = NULL;
   commandBufferAllocateInfo.commandPool = m_GraphicPool;
   commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

   if (vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
       std::cout << "Command buffer creation failed !" << '\n';
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame, uint32_t imageIndex, ImDrawData* draw_data)
{
    VkFramebuffer framebuffers = m_SwapChain.GetFramebuffers()[currentFrame];
    auto GBufferDescriptorSet = m_GBufferDescriptor.GetDescriptorSets()[currentFrame];

    VkDescriptorSet perMeshDescriptorSet = VK_NULL_HANDLE;
    if (!m_PerMeshDescriptor.GetDescriptorSets().empty())
        perMeshDescriptorSet = m_PerMeshDescriptor.GetDescriptorSets()[currentFrame];
    auto perPassDescriptorSet = m_PerPassDescriptor.GetDescriptorSets()[currentFrame];

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = NULL;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL; //ignored if VK_COMMAND_BUFFER_LEVEL_PRIMARY

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        std::cout << "Failed to begin commandBuffer !" << '\n';

    std::array<VkClearValue, 7> clearValues{};
    clearValues[0].color = clearColor;
    clearValues[1].color = clearColor;
    clearValues[2].color = clearColor;
    clearValues[3].color = clearColor;
    clearValues[4].color = clearColor;
    clearValues[5].depthStencil = { 1.0f, 0 };
    clearValues[6].color = clearColor;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass.getRenderPass();
    renderPassInfo.framebuffer = framebuffers;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_SwapChain.GetExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    auto extent = m_SwapChain.GetExtent();

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineCubeMap);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineCubeMapLayout, 0, 1, &perPassDescriptorSet, 0, NULL);

    m_CubeMap->BindVertexBuffer(commandBuffer);
    m_CubeMap->BindIndexBuffer(commandBuffer);

    vkCmdDrawIndexed(commandBuffer, 36, 1, 0, 0, 0);

    if (m_Wireframe)
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineFirstPassLineMode);
    else
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineFirstPass);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

    uint64_t bufferSize = sizeof(glm::mat4);
    uint64_t paddedSize = (bufferSize + minSize - 1) & ~(minSize - 1);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineFirstPassLayout, 0, 1, &perPassDescriptorSet, 0, NULL);

    for (size_t i = 0; i < m_Meshes.size(); i++)
    {
        auto mesh = m_Meshes[i];

        uint32_t offset = static_cast<uint32_t>(i * paddedSize);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineFirstPassLayout, 1, 1, &perMeshDescriptorSet, 1, &offset);
        mesh->BindVertexBuffer(commandBuffer);
        mesh->BindIndexBuffer(commandBuffer);

        for (const auto& primitive : mesh->GetPrimitives())
        {
            m_Materials.BindMaterial(primitive.materialID, commandBuffer, m_GraphicPipelineFirstPassLayout);

            vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, static_cast<int32_t>(primitive.vertexOffset), 0);
        }
    }

    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineSecondPass);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineSecondPassLayout, 0, 1, &perPassDescriptorSet, 0, NULL);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineSecondPassLayout, 1, 1, &GBufferDescriptorSet, 0, NULL);
    m_simpleQuadMesh.BindVertexBuffer(commandBuffer);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    /*
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // SrcStageMask (render pass)
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,   // DstStageMask (ray tracing)
        0,
        0, nullptr,
        0, nullptr,
        0, nullptr);
        */

    if (*m_RayTracingAccelerationStructure->GetActiveRaytracingPtr())
        m_RayTracingAccelerationStructure->RecordCommandBuffer(m_Device, commandBuffer, currentFrame, extent.width, extent.height);

    VkImageMemoryBarrier imageBarrier = {};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;                   // Layout actuel après le ray tracing
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // Layout souhaité pour la render pass
    imageBarrier.srcQueueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();
    imageBarrier.dstQueueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();
    imageBarrier.image = m_SwapChain.GetImages()[currentFrame];  // L'image générée par le ray tracing
    imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange.baseMipLevel = 0;
    imageBarrier.subresourceRange.levelCount = 1;
    imageBarrier.subresourceRange.baseArrayLayer = 0;
    imageBarrier.subresourceRange.layerCount = 1;
    imageBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;  // Accès en écriture par le ray tracing
    imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;   // Accès en lecture pour la subpass

    VkPipelineStageFlagBits stage = *m_RayTracingAccelerationStructure->GetActiveRaytracingPtr() ? VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,  // Étape de ray tracing en source
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         // Étape de fragment shader pour la render pass
        0,
        0, nullptr,
        0, nullptr,
        1, &imageBarrier
    );

    VkFramebuffer finalFramebuffers = m_SwapChain.GetFinalFramebuffers()[imageIndex];

    std::array<VkClearValue, 1> finalClearValues{};
    finalClearValues[0].color = clearColor;

    VkRenderPassBeginInfo finalRenderPassInfo{};
    finalRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    finalRenderPassInfo.renderPass = m_FinalRenderPass.getRenderPass();
    finalRenderPassInfo.framebuffer = finalFramebuffers;
    finalRenderPassInfo.renderArea.offset = { 0, 0 };
    finalRenderPassInfo.renderArea.extent = m_SwapChain.GetExtent();
    finalRenderPassInfo.clearValueCount = static_cast<uint32_t>(finalClearValues.size());
    finalRenderPassInfo.pClearValues = finalClearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &finalRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        std::cout << "Failed to end command buffer !" << '\n';
}

void Renderer::CleanupCommandBuffers() const
{
    if (m_Device != VK_NULL_HANDLE && m_GraphicPool != VK_NULL_HANDLE)
    {
        for (const auto& commandBuffer : m_CommandBuffers)
            vkFreeCommandBuffers(m_Device, m_GraphicPool, 1, &commandBuffer);
    }
}

void Renderer::CreateSyncObject()
{
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {

            std::cout << "Sync object creation failed !" << '\n';
        }
    }
}

bool Renderer::WindowShouldClose()
{
    return glfwWindowShouldClose(m_Window.getWindow());
}

void Renderer::Draw()
{
    glfwPollEvents();

    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain.GetSwapChain(), UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_SwapChain.RebuildSwapChain(m_PhysicalDevice, m_Allocator, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);
        m_RayTracingAccelerationStructure->UpdateImageDescriptor(m_Device, m_SwapChain.GetImageViews());
        CleanupCommandBuffers();
        m_DepthImage.Cleanup(m_Allocator, m_Device);
        CreateDepthRessources();
        auto extent = m_SwapChain.GetExtent();
        m_GBuffer.ReBuildGBuffer(MAX_FRAMES_IN_FLIGHT, extent.width, extent.height, m_Allocator, m_Device, { m_QueueFamilyIndices.graphicsFamily.value() });
        m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_FinalRenderPass.getRenderPass(), m_GBuffer, m_DepthImage.GetImageView());
        CreateCommandBuffers();
        UpdateGBufferDescriptor();
        m_SwapChain.CreateImGuiImageDescriptor(m_Device);
        return;
    }
    else if (result != VK_SUCCESS) {
        std::cout << "Failed to acquire next image !" << '\n';
    }

    m_DeltaTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_LastTime).count();
    m_LastTime = std::chrono::high_resolution_clock::now();
    ProcessKeyInput();

    m_Camera->UpdatePosition(static_cast<float>(m_DeltaTime));
    UpdateUniform();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::DockSpaceOverViewport();
    }

    {
        ImGui::Begin("Tools");

        ImGui::Checkbox("Wireframe", &m_Wireframe);

        ImGui::Checkbox("RayTracing", m_RayTracingAccelerationStructure->GetActiveRaytracingPtr());

        ImGui::SliderFloat("CameraSpeed", m_Camera->GetSpeed(), 0., 100.);

        ImGui::End();
    }

    {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs;
        
        ImGui::Begin("Scene", 0, window_flags);
        {
            ImGui::BeginChild("GameRender", ImVec2(0,0), 0, window_flags);

            //float width = ImGui::GetContentRegionAvail().x;
            //float height = ImGui::GetContentRegionAvail().y;

            ImGui::Image(
                (ImTextureID)m_SwapChain.GetImGuiImageDescriptor(m_CurrentFrame),
                ImGui::GetContentRegionAvail(),
                ImVec2(0, 0),
                ImVec2(1, 1)
            );

            if (!gameViewportHovered)
            {
                gameViewportHovered = ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
            }
            else
            {
                if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    gameViewportHovered = false;
                    InitKeyPressedMap();
                }
            }
        }
        ImGui::EndChild();
        ImGui::End();
    }

    ImGui::Render();
    ImDrawData* main_draw_data = ImGui::GetDrawData();

    vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);

    RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], m_CurrentFrame, imageIndex, main_draw_data);

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };

    VkSubmitInfo submitsInfo;
    submitsInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitsInfo.pNext = NULL;
    submitsInfo.waitSemaphoreCount = 1;
    submitsInfo.pWaitSemaphores = waitSemaphores;
    submitsInfo.pWaitDstStageMask = waitStages;
    submitsInfo.commandBufferCount = 1;
    submitsInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];
    submitsInfo.signalSemaphoreCount = 1;
    submitsInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

    // Soumettre les commandes
    if (vkQueueSubmit(m_GraphicsQueue, 1, &submitsInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
        std::cout << "Failed to submit draw command buffer!" << '\n';
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_SwapChain.GetSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optionnel

    result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
    {
        m_SwapChain.RebuildSwapChain(m_PhysicalDevice, m_Allocator, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);
        m_RayTracingAccelerationStructure->UpdateImageDescriptor(m_Device, m_SwapChain.GetImageViews());
        CleanupCommandBuffers();
        m_DepthImage.Cleanup(m_Allocator, m_Device);
        CreateDepthRessources();
        auto extent = m_SwapChain.GetExtent();
        m_GBuffer.ReBuildGBuffer(MAX_FRAMES_IN_FLIGHT, extent.width, extent.height, m_Allocator, m_Device, { m_QueueFamilyIndices.graphicsFamily.value() });
        m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_FinalRenderPass.getRenderPass(), m_GBuffer, m_DepthImage.GetImageView());
        CreateCommandBuffers();
        UpdateGBufferDescriptor();
        m_SwapChain.CreateImGuiImageDescriptor(m_Device);
        m_FramebufferResized = false;
        return;
    }
    else if (result != VK_SUCCESS) {
        std::cout << "Failed to present image!" << '\n';
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::UpdateUniform()
{
    m_SceneUniform.view = m_Camera->GetView();
    m_SceneUniform.projection = m_Camera->GetProjection();
    m_SceneUniform.position = m_Camera->GetPosition();
    m_SceneUniform.numDirectionalLights = static_cast<int>(m_DirectionalLights.size());
    m_SceneUniform.numPointLights = static_cast<int>(m_PointLights.size());

    memcpy(m_PerPassDescriptor.GetUniformAllocationInfos()[m_CurrentFrame].pMappedData, &m_SceneUniform, sizeof(SceneUniform));

    std::vector<VmaAllocationInfo> lightsAllocationInfos;
    m_PerPassDescriptor.GetUniformStorageBufferAllocationInfos(lightsAllocationInfos);

    std::vector<UniformDirectionalLight> uniformDirectionalLights;
    uniformDirectionalLights.reserve(m_DirectionalLights.size());
    for (DirectionalLight& directionalLight : m_DirectionalLights)
        uniformDirectionalLights.emplace_back(directionalLight.GetUniformDirectionalLight());
    memcpy(lightsAllocationInfos[static_cast<size_t>(2) * m_CurrentFrame].pMappedData, uniformDirectionalLights.data(), sizeof(UniformDirectionalLight) * m_DirectionalLights.size());

    std::vector<UniformPointLight> uniformPointLights;
    uniformPointLights.reserve(m_PointLights.size());
    for (PointLight& pointLight : m_PointLights)
        uniformPointLights.emplace_back(pointLight.GetUniformPointLight());
    memcpy(lightsAllocationInfos[static_cast<size_t>(2) * m_CurrentFrame + 1].pMappedData, uniformPointLights.data(), sizeof(UniformPointLight) * m_PointLights.size());

    if (*m_RayTracingAccelerationStructure->GetActiveRaytracingPtr())
        m_RayTracingAccelerationStructure->UpdateUniform(glm::inverse(m_Camera->GetView()), glm::inverse(m_Camera->GetProjection()), m_CurrentFrame, m_Meshes);

    if (!m_Meshes.empty())
    {

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

        uint64_t paddedSize = (sizeof(glm::mat4) + minSize - 1) & ~(minSize - 1);;

        char* modelsData = static_cast<char*>(malloc(m_Meshes.size() * paddedSize));

        m_Meshes[0]->SetModel(glm::rotate(m_Meshes[0]->GetModel(), glm::radians<float>(static_cast<float>(m_DeltaTime) * 32.36f), glm::vec3(0., 1., 0.)));

        m_Meshes.back()->SetModel(glm::rotate(m_Meshes.back()->GetModel(), glm::radians<float>(static_cast<float>(m_DeltaTime) * 32.36f), glm::vec3(0., 1., 0.)));
        
        //m_RayTracingAccelerationStructure->UpdateTransform(m_Device, m_Allocator, m_ComputeQueue, m_ComputePool, 0, m_Meshes[0]->GetModel());

        for (size_t i = 0; i < m_Meshes.size(); i++)
        {
            uint64_t bufferSize = sizeof(glm::mat4);
            char* p = modelsData + i * paddedSize;
            memcpy(p, &(m_Meshes[i]->GetModel()), bufferSize);
        }

        memcpy(m_PerMeshDescriptor.GetUniformAllocationInfos()[m_CurrentFrame].pMappedData, modelsData, m_Meshes.size() * paddedSize);

        free(modelsData);
    }
}

Camera* Renderer::GetCamera() const
{
    return m_Camera;
}

void Renderer::InitKeyPressedMap()
{
    for (int i = 32; i < 349; i++)
    {
        m_KeyPressedMap[i].previous = false;
        m_KeyPressedMap[i].current = false;
    }
}

std::map<int, KeyPress>* Renderer::GetKeyPressedMap()
{
    return &m_KeyPressedMap;
}

void Renderer::ProcessKeyInput()
{
    float fDeltaTime = static_cast<float>(m_DeltaTime);

    if (m_KeyPressedMap[GLFW_KEY_W].current)
        m_Camera->Move(FORWARD, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_A].current)
        m_Camera->Move(LEFT, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_S].current)
        m_Camera->Move(BACKWARD, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_D].current)
        m_Camera->Move(RIGHT, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_LEFT_SHIFT].current)
        m_Camera->Move(DOWN, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_SPACE].current)
        m_Camera->Move(UP, fDeltaTime);
    if (m_KeyPressedMap[GLFW_KEY_ESCAPE].current)
        glfwSetWindowShouldClose(m_Window.getWindow(), true);

    for (int i = 32; i < 349; i++)
        m_KeyPressedMap[i].previous = m_KeyPressedMap[i].current;
}

bool Renderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool Renderer::isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties{};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures{};
    physicalDeviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &physicalDeviceBufferDeviceAddressFeatures;
    
    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = m_SwapChain.QuerySupportDetails(device, m_Surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    FindQueueFamilies(device, m_Surface);

    std::cout << "SamplerAnisotropy: " << deviceFeatures2.features.samplerAnisotropy << "\nFillModeNonSolid: " << deviceFeatures2.features.fillModeNonSolid << "\nBufferDeviceAddress: " << physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress << "\n";

    bool suitable = m_QueueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures2.features.samplerAnisotropy && deviceFeatures2.features.fillModeNonSolid && physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress;

    return suitable;
}
