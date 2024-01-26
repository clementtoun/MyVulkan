#include "Renderer.h"
#include <array>

#undef CreateWindow

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) 
{
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->m_FramebufferResized = true;

    auto camera = renderer->GetCamera();
    camera->SetAspect(width / (double) height);
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) 
{
    Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));

    int width, heigth;
    glfwGetWindowSize(window, &width, &heigth);

    bool leftClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

    auto camera = renderer->GetCamera();
    camera->ProcessMouseMouve(xpos / (double)width, ypos / (double)heigth, leftClick);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_REPEAT)
    {
        Renderer* renderer = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));

        std::map<int, bool>* keyPressMap = renderer->GetKeyPressedMap();

        (*keyPressMap)[key] = action;
    }
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
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

    CreateInstance(ApplicationName, ApplicationVersion, EngineName, EngineVersion);

    CreateSurface();

    SelectPhysicalDevice();

    CreateLogicalDevice();

    CreateCommandPool();

    CreateVmaAllocator();

    m_SwapChain.BuildSwapChain(m_PhysicalDevice, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);

    CreateDepthRessources();

    CreateRenderPass();

    m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_DepthImage.GetImageView());

    Shader vertexShader;
    vertexShader.createModule(m_Device, "vert.spv");
    Shader fragmentShader;
    fragmentShader.createModule(m_Device, "frag.spv");

    auto multiTri = new Mesh({ { {{0.0,-0.5,0.0}, {1.0,0.0,0.0}}, {{-0.5,0.5,0.0},{0.0,1.0,0.0}}, {{0.5,0.5,0.0}, {0.0,0.0,1.0}} } }, { 0,1,2 });
    multiTri->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
    multiTri->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
    multiTri->SetModel(glm::translate(glm::mat4(1.), glm::vec3(0.5, 0.0, 0.0)));

    auto fireTri = new Mesh({ { {{0.0,-0.5,0.0}, {1.0,0.0,0.0}}, {{-0.5,0.5,0.0},{1.0,0.75,0.25}}, {{0.5,0.5,0.0}, {1.0,0.5,0.0}} } }, { 0,1,2 });
    fireTri->CreateVertexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
    fireTri->CreateIndexBuffers(m_Allocator, m_Device, m_TransferPool, m_TranferQueue, m_QueueFamilyIndices.transferFamily.value(), m_QueueFamilyIndices.graphicsFamily.value());
    fireTri->SetModel(glm::translate(glm::mat4(1.), glm::vec3(-0.25, -0.25, 0.5)));

    m_Meshes.push_back(multiTri);
    m_Meshes.push_back(fireTri);

    CreatePerMeshDescriptor();

    CreateGraphicPipeline(vertexShader, fragmentShader);

    vertexShader.cleanup(m_Device);
    fragmentShader.cleanup(m_Device);

    CreateCommandBuffers();
    for (int i = 0; i < m_SwapChain.GetFramebuffers().size(); i++)
        RecordCommandBuffer(m_CommandBuffers[i], i);

    CreateSyncObject();

    auto extent = m_SwapChain.GetExtent();
    m_Camera = new EulerCamera(glm::vec3(0., 0., 2.), glm::vec3(0.), glm::vec3(0., 1., 0.), 45., extent.width / (double)extent.height, 0.001, 100.);
    m_Camera->SetSpeed(10.);
    m_Camera->SetMouseSensibility(1.);
}

Renderer::~Renderer()
{
    if (m_Camera)
        delete m_Camera;

    vkQueueWaitIdle(m_GraphicsQueue);

    if (m_Device != VK_NULL_HANDLE)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }
    }

    CleanupCommandBuffers();

    if (m_Device != VK_NULL_HANDLE)
    {
        for (auto mesh : m_Meshes)
        {
            mesh->DestroyBuffer(m_Allocator, m_Device);
            delete mesh;
        }
    }

    if (m_Device != VK_NULL_HANDLE && m_TransferPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_Device, m_TransferPool, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(m_Device, m_GraphicPool, NULL);

    if (m_Device != VK_NULL_HANDLE)
    {
        m_PerMeshDescriptor.DestroyDescriptorPool(m_Device);
        m_PerMeshDescriptor.DestroyDescriptorSetLayout(m_Device);
        m_PerMeshDescriptor.DestroyUniformBuffer(m_Allocator, m_Device);
    }

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(m_Device, m_GraphicPipeline, NULL);

    if (m_Device != VK_NULL_HANDLE && m_GraphicPipelineLayout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(m_Device, m_GraphicPipelineLayout, NULL);

    if (m_Device != VK_NULL_HANDLE)
        m_RenderPass.cleanup(m_Device);

    if (m_Device != VK_NULL_HANDLE)
        m_DepthImage.Cleanup(m_Allocator, m_Device);

    if (m_Device != VK_NULL_HANDLE)
        m_SwapChain.Cleanup(m_Device);

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
    if (!m_Window.createWindow(WindowName.c_str(), width, height))
        std::cout << "GLFW Window creation failed !" << std::endl;
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
        std::cout << "Vulkan instance creation failed !" << std::endl;
}

void Renderer::CreateSurface()
{
    if (m_Window.createSurface(m_Instance, m_Surface) != VK_SUCCESS)
        std::cout << "Vulkan surface creation failed !" << std::endl;
}

void Renderer::SelectPhysicalDevice()
{
    uint32_t physicalDeviceCount;

    if (vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, NULL) != VK_SUCCESS)
    {
        std::cout << "No physical device found !" << std::endl;
    }
    else if (physicalDeviceCount <= 0)
    {
        std::cout << "No physical device found !" << std::endl;
    }

    std::vector<VkPhysicalDevice> vkPysicalDevices(physicalDeviceCount);

    if (vkEnumeratePhysicalDevices(m_Instance, &physicalDeviceCount, vkPysicalDevices.data()) != VK_SUCCESS)
    {
        std::cout << "Failed to get physical device !" << std::endl;
    }

    for (const auto& pysicalDevice : vkPysicalDevices) {
        if (isDeviceSuitable(pysicalDevice)) {
            m_PhysicalDevice = pysicalDevice;
            break;
        }
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        std::cout << "No GPU can run this program!" << std::endl;
    }
}

void Renderer::CreateVmaAllocator()
{
    VmaVulkanFunctions vulkanFunctions = {};
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

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
    allocatorCreateInfo.device = m_Device;
    allocatorCreateInfo.instance = m_Instance;
    //allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    if (vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator) != VK_SUCCESS)
        std::cout << "Vma allocator creation failed !" << std::endl;
}

void Renderer::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices queueFamilyIndices;

    // Code pour trouver les indices de familles à ajouter à la structure
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

        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
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
    std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.graphicsFamily.value(), m_QueueFamilyIndices.presentFamily.value(), m_QueueFamilyIndices.transferFamily.value() };

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

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo vkDeviceCreateInfo;
    vkDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    vkDeviceCreateInfo.flags = 0;
    vkDeviceCreateInfo.pNext = NULL;
    vkDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
    vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    vkDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    vkDeviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
    vkDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    vkDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (vkCreateDevice(m_PhysicalDevice, &vkDeviceCreateInfo, NULL, &m_Device) != VK_SUCCESS)
    {
        std::cout << "Logical device creation failed !" << std::endl;
    }

    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.graphicsFamily.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.presentFamily.value(), 0, &m_PresentQueue);
    vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.transferFamily.value(), 0, &m_TranferQueue);

    if (m_GraphicsQueue == VK_NULL_HANDLE)
    {
        std::cout << "Graphics queue retrieve failed !" << std::endl;
    }
    if (m_PresentQueue == VK_NULL_HANDLE)
    {
        std::cout << "Present queue retrieve failed !" << std::endl;
    }
    if (m_TranferQueue == VK_NULL_HANDLE)
    {
        std::cout << "Tranfer queue retrieve failed !" << std::endl;
    }
}

void Renderer::CreateDepthRessources()
{
    VkFormat depthStencilFormat = Image::findDepthFormat(m_PhysicalDevice);

    m_DepthImage.CreateImage(m_Allocator, m_Device, m_SwapChain.GetExtent().width, m_SwapChain.GetExtent().height,
        depthStencilFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, { m_QueueFamilyIndices.graphicsFamily.value() });
    m_DepthImage.CreateImageView(m_Device, depthStencilFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::CreateRenderPass()
{
    VkAttachmentDescription colorAttachmentDescription;
    colorAttachmentDescription.flags = 0;
    colorAttachmentDescription.format = m_SwapChain.GetSurfaceFormat();
    colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

    VkAttachmentReference attachmentReference;
    attachmentReference.attachment = 0;
    attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    m_RenderPass.addSubPass(VK_PIPELINE_BIND_POINT_GRAPHICS, {}, { attachmentReference }, {}, { depthAttachmentRef }, {});

    m_RenderPass.addSubPassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

    m_RenderPass.CreateRenderPass(m_Device, { colorAttachmentDescription, depthAttachmentDescription });
}

void Renderer::CreatePerMeshDescriptor()
{
    uint32_t numMesh = m_Meshes.size();

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
    descriptorSetLayoutBinding.binding = 0;
    descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorSetLayoutBinding.descriptorCount = 1;
    descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptorSetLayoutBinding.pImmutableSamplers = NULL;

    m_PerMeshDescriptor.CreateDescriptorSetLayout(m_Device, { descriptorSetLayoutBinding }, 0);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

    uint64_t bufferSize = sizeof(glm::mat4);
    uint64_t paddedSize = minSize - bufferSize + bufferSize;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        m_PerMeshDescriptor.AddUniformBuffer(m_Allocator, m_Device, m_Meshes.size() * paddedSize);
    }

    VkDescriptorPoolSize descriptorPoolSize{};
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
        for (int j = 0; j < m_Meshes.size(); j++)
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

void Renderer::CreatePerPassDescriptor()
{
}

void Renderer::CreateGraphicPipeline(Shader& vertexShader, Shader& fragmentShader)
{
    VkPipelineShaderStageCreateInfo pipelineVertexShaderStageCreateInfo;
    pipelineVertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineVertexShaderStageCreateInfo.pNext = NULL;
    pipelineVertexShaderStageCreateInfo.flags = 0;
    pipelineVertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineVertexShaderStageCreateInfo.module = vertexShader.getShaderModule();
    pipelineVertexShaderStageCreateInfo.pName = "main";
    pipelineVertexShaderStageCreateInfo.pSpecializationInfo = NULL;

    VkPipelineShaderStageCreateInfo pipelineFragmentShaderStageCreateInfo;
    pipelineFragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineFragmentShaderStageCreateInfo.pNext = NULL;
    pipelineFragmentShaderStageCreateInfo.flags = 0;
    pipelineFragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipelineFragmentShaderStageCreateInfo.module = fragmentShader.getShaderModule();
    pipelineFragmentShaderStageCreateInfo.pName = "main";
    pipelineFragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

    std::array<VkPipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos = { pipelineVertexShaderStageCreateInfo, pipelineFragmentShaderStageCreateInfo };

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
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
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

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo;
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.pNext = NULL;
    pipelineColorBlendStateCreateInfo.flags = 0;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    pipelineColorBlendStateCreateInfo.attachmentCount = 1;
    pipelineColorBlendStateCreateInfo.pAttachments = &pPipelineColorBlendAttachmentState;
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
    
    auto layout = m_PerMeshDescriptor.GetDescriptorSetLayout();

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.pNext = NULL;
    pipelineLayoutCreateInfo.flags = 0;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &layout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    if (vkCreatePipelineLayout(m_Device, &pipelineLayoutCreateInfo, NULL, &m_GraphicPipelineLayout) != VK_SUCCESS)
        std::cout << "Pipeline layout creation failed !" << std::endl;

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = NULL;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(pipelineShaderStageCreateInfos.size());
    graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos.data();
    graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = &pipelineTessellationStateCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
    graphicsPipelineCreateInfo.layout = m_GraphicPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = m_RenderPass.getRenderPass();
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, NULL, &m_GraphicPipeline) != VK_SUCCESS)
        std::cout << "Pipeline cration failed !" << std::endl;
}

void Renderer::CreateCommandPool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_GraphicPool) != VK_SUCCESS) {
        std::cout << "Graphic command pool creation failed !" << std::endl;
    }

    poolInfo.queueFamilyIndex = m_QueueFamilyIndices.transferFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT; 

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_TransferPool) != VK_SUCCESS) {
        std::cout << "Transfer command pool creation failed" << std::endl;
    }
}

void Renderer::CreateCommandBuffers() 
{
   m_CommandBuffers.resize(m_SwapChain.GetFramebuffers().size());

   VkCommandBufferAllocateInfo commandBufferAllocateInfo;
   commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   commandBufferAllocateInfo.pNext = NULL;
   commandBufferAllocateInfo.commandPool = m_GraphicPool;
   commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

   if (vkAllocateCommandBuffers(m_Device, &commandBufferAllocateInfo, m_CommandBuffers.data()) != VK_SUCCESS)
       std::cout << "Command buffer creation failed !" << std::endl;
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkFramebuffer framebuffers = m_SwapChain.GetFramebuffers()[imageIndex];

    auto descriptorSet = m_PerMeshDescriptor.GetDescriptorSets()[m_CurrentFrame];

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = NULL;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL; //ignored if VK_COMMAND_BUFFER_LEVEL_PRIMARY

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        std::cout << "Failed to begin commandBuffer !" << std::endl;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.5f, 0.7f, 1.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass.getRenderPass();
    renderPassInfo.framebuffer = framebuffers;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_SwapChain.GetExtent();
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipeline);

    auto extent = m_SwapChain.GetExtent();

    VkViewport viewport;
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor;
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

    uint64_t bufferSize = sizeof(glm::mat4);
    uint64_t paddedSize = minSize - bufferSize + bufferSize;

    for (int i = 0; i < m_Meshes.size(); i++)
    {
        auto mesh = m_Meshes[i];

        uint32_t offset = i * paddedSize;

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicPipelineLayout, 0, 1, &descriptorSet, 1, &offset);
        mesh->BindVertexBuffer(commandBuffer);
        mesh->BindIndexBuffer(commandBuffer);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->GetIndexes().size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        std::cout << "Failed to end command buffer !" << std::endl;
}

void Renderer::CleanupCommandBuffers()
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
    m_ImagesInFlight.resize(m_SwapChain.GetImages().size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {

            std::cout << "Sync object creation failed !" << std::endl;
        }
    }
}

bool Renderer::WindowShouldClose()
{
    return glfwWindowShouldClose(m_Window.getWindow());
}

void Renderer::Draw()
{
    m_DeltaTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_LastTime).count();
    m_LastTime = std::chrono::high_resolution_clock::now();
    glfwPollEvents();
    ProcessKeyInput();

    vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;

    VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain.GetSwapChain(), UINT64_MAX,
        m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        m_SwapChain.RebuildSwapChain(m_PhysicalDevice, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);
        CleanupCommandBuffers();
        m_DepthImage.Cleanup(m_Allocator, m_Device);
        CreateDepthRessources();
        m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_DepthImage.GetImageView());
        CreateCommandBuffers();
        for (int i = 0; i < m_SwapChain.GetFramebuffers().size(); i++)
            RecordCommandBuffer(m_CommandBuffers[i], i);
        return;
    }
    else if (result != VK_SUCCESS) {
        std::cout << "Failed to acquire next image !" << std::endl;
    }

    UpdateUniform();

    if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Device, 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

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
    submitsInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];
    submitsInfo.signalSemaphoreCount = 1;
    submitsInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

    vkQueueSubmit(m_GraphicsQueue, 1, &submitsInfo, m_InFlightFences[m_CurrentFrame]);

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
        m_SwapChain.RebuildSwapChain(m_PhysicalDevice, m_Device, m_Surface, m_Window.getWindow(), m_QueueFamilyIndices);
        CleanupCommandBuffers();
        m_DepthImage.Cleanup(m_Allocator, m_Device);
        CreateDepthRessources();
        m_SwapChain.CreateFramebuffer(m_Device, m_RenderPass.getRenderPass(), m_DepthImage.GetImageView());
        CreateCommandBuffers();
        for (int i = 0; i < m_SwapChain.GetFramebuffers().size(); i++)
            RecordCommandBuffer(m_CommandBuffers[i], i);
        m_FramebufferResized = false;
        return;
    }
    else if (result != VK_SUCCESS) {
        std::cout << "Failed to present image!" << std::endl;
    }

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    glfwSwapBuffers(m_Window.getWindow());
}

void Renderer::UpdateUniform()
{
    /*
    m_Mvp.model = glm::mat4(1.0);
    m_Mvp.view = m_Camera->GetView();
    m_Mvp.projection = m_Camera->GetProjection();

    auto uniformAlloc = m_PerMeshDescriptor.GetUniformAllocation()[m_CurrentFrame];

    void* data;
    vmaMapMemory(m_Allocator, uniformAlloc, &data);
    memcpy(data, &m_Mvp, sizeof(MVP));
    vmaUnmapMemory(m_Allocator, uniformAlloc);
    */

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
    VkDeviceSize minSize = properties.limits.minUniformBufferOffsetAlignment;

    uint64_t bufferSize = sizeof(glm::mat4);
    uint64_t paddedSize = minSize - bufferSize + bufferSize;

    auto uniformAllocInfo = m_PerMeshDescriptor.GetUniformAllocationInfos()[m_CurrentFrame];

    char* modelsData = (char*) malloc(m_Meshes.size() * paddedSize);

    for(int i = 0; i < m_Meshes.size(); i++)
    {
        char* p = modelsData + i * paddedSize;
        memcpy(p, &m_Meshes[i]->GetModel(), sizeof(glm::mat4));
    }

    memcpy(uniformAllocInfo.pMappedData, modelsData, m_Meshes.size() * paddedSize);

    free(modelsData);
}

Camera* Renderer::GetCamera()
{
    return m_Camera;
}

void Renderer::InitKeyPressedMap()
{
    for (int i = 32; i < 349; i++)
    {
        m_IsKeyPressedMap[i] = false;
    }
}

std::map<int, bool>* Renderer::GetKeyPressedMap()
{
    return &m_IsKeyPressedMap;
}

void Renderer::ProcessKeyInput()
{
    if (m_IsKeyPressedMap[GLFW_KEY_W])
        m_Camera->Move(FORWARD, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_A])
        m_Camera->Move(LEFT, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_S])
        m_Camera->Move(BACKWARD, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_D])
        m_Camera->Move(RIGHT, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_LEFT_SHIFT])
        m_Camera->Move(UP, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_SPACE])
        m_Camera->Move(DOWN, m_DeltaTime);
    if (m_IsKeyPressedMap[GLFW_KEY_ESCAPE])
        glfwSetWindowShouldClose(m_Window.getWindow(), true);
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
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = m_SwapChain.QuerySupportDetails(device, m_Surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    FindQueueFamilies(device, m_Surface);

    bool suitable = m_QueueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;

    return suitable;
}
