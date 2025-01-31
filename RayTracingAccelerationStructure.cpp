#include "RayTracingAccelerationStructure.h"
#include <iostream>

RayTracingAccelerationStructure::RayTracingAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes, const std::vector<VkImageView>& imageViews, const std::vector<VkDescriptorSetLayout>& layouts)
{
    m_RayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 deviceProperties2{};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &m_RayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
    
    vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
    vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
    vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
    vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
    vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
    vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

    m_UpdateScratchBuffers.resize(imageViews.size());

    CreateBottomLevelASs(device, vmaAllocator, computeQueue, computePool, meshes);
    CreateTopLevelAS(device, vmaAllocator, computeQueue, computePool, meshes, static_cast<int>(imageViews.size()));

    CreateDescriptorSets(device, vmaAllocator, imageViews);
    CreateRayTracingPipeline(device, layouts);
    CreateShaderBindingTable(device, vmaAllocator);
}

void RayTracingAccelerationStructure::BindPipeline(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RayTracingPipeline);
}

void RayTracingAccelerationStructure::BindTopLevelASDescriptorSet(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_RayTracingPipelineLayout, 0, 1, &m_TopLevelASDescriptor.GetDescriptorSets()[imageIndex], 0, 0);
}

void RayTracingAccelerationStructure::Cleanup(VkDevice device, VmaAllocator vmaAllocator)
{
    vmaDestroyBuffer(vmaAllocator, m_RayGenShaderBindingTable.buffer, m_RayGenShaderBindingTable.memory);
    vmaDestroyBuffer(vmaAllocator, m_MissShaderBindingTable.buffer, m_MissShaderBindingTable.memory);
    vmaDestroyBuffer(vmaAllocator, m_HitShaderBindingTable.buffer, m_HitShaderBindingTable.memory);

    vkDestroyPipelineLayout(device, m_RayTracingPipelineLayout, NULL);
    vkDestroyPipeline(device, m_RayTracingPipeline, NULL);

    m_TopLevelASDescriptor.DestroyUniformBuffer(vmaAllocator, device);
    m_TopLevelASDescriptor.DestroyDescriptorPool(device);
    m_TopLevelASDescriptor.DestroyDescriptorSetLayout(device);

    for (const auto& scratchBuffer : m_UpdateScratchBuffers)
    {
        if (scratchBuffer.handle != VK_NULL_HANDLE)
            DeleteScratchBuffer(vmaAllocator, scratchBuffer);
    }
    
    m_ShaderGroups.clear();
    
    for (auto& bottomLevelAS : m_BottomLevelASs)
    {
        vmaDestroyBuffer(vmaAllocator, bottomLevelAS.buffer, bottomLevelAS.memory);
        vkDestroyAccelerationStructureKHR(device, bottomLevelAS.handle, nullptr);
    }

    for (AccelerationStructure& topLevelAS : m_TopLevelAS)
    {
        vmaDestroyBuffer(vmaAllocator, topLevelAS.buffer, topLevelAS.memory);
        vkDestroyAccelerationStructureKHR(device, topLevelAS.handle, nullptr);
    }

    m_TopLevelAS.clear();

    for (InstanceBuffer& instanceBuffer : m_InstanceBuffer)
    {
        if (instanceBuffer.buffer != VK_NULL_HANDLE && instanceBuffer .memory != VK_NULL_HANDLE)
            vmaDestroyBuffer(vmaAllocator, instanceBuffer.buffer, instanceBuffer.memory);
    }
}

void RayTracingAccelerationStructure::CreateBottomLevelASs(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes)
{
    m_BottomLevelASs.reserve(meshes.size());
    
    for (auto& mesh : meshes)
    {
        AccelerationStructure bottomLevelAS;
        
        std::vector<VkAccelerationStructureGeometryKHR> accelerationStructureGeometry{};
        mesh->GetAccelerationStructureGeometrys(device, accelerationStructureGeometry);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(accelerationStructureGeometry.size());
        accelerationStructureBuildGeometryInfo.pGeometries = accelerationStructureGeometry.data();

        std::vector<uint32_t> primitivesTrianglesCounts;
        mesh->GetPrimitvesTrianglesCounts(primitivesTrianglesCounts);
        
        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            primitivesTrianglesCounts.data(),
            &accelerationStructureBuildSizesInfo);

        CreateAccelerationStructureBuffer(device, vmaAllocator, bottomLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &bottomLevelAS.handle) != VK_SUCCESS)
        {
            std::cout << "Fail to create Acceleration Structure !" << "\n";
            continue;
        }

        RayTracingScratchBuffer scratchBuffer = CreateScratchBuffer(device, vmaAllocator, accelerationStructureBuildSizesInfo.buildScratchSize);
        
        accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationStructureBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.handle;
        accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        std::vector<VkAccelerationStructureBuildRangeInfoKHR> accelerationStructureRangeInfos;
        mesh->GetAccelerationStructureRangeInfos(accelerationStructureRangeInfos);
        
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureRangeInfosPtr;
        accelerationStructureRangeInfosPtr.reserve(accelerationStructureRangeInfos.size());
        for (uint32_t i = 0; i < accelerationStructureRangeInfos.size(); i++)
            accelerationStructureRangeInfosPtr.emplace_back(&accelerationStructureRangeInfos[i]);

        VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, computePool);
    
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureRangeInfosPtr.data());

        VulkanUtils::EndSingleTimeCommands(device, computePool, computeQueue, commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.handle;
        bottomLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

        if (bottomLevelAS.deviceAddress == 0)
            std::cout << "Failed to get device address for bottom level acceleration structure!\n";

        m_BottomLevelASs.emplace_back(bottomLevelAS);

        DeleteScratchBuffer(vmaAllocator, scratchBuffer);
    }
}

void RayTracingAccelerationStructure::CreateTopLevelAS(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes, int count)
{
    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(m_BottomLevelASs.size());
    
    for (size_t i = 0; i < m_BottomLevelASs.size(); i++)
    {
        glm::mat4 model = meshes[i]->GetModel();

        VkTransformMatrixKHR transformMatrix = {
            model[0][0], model[1][0], model[2][0], model[3][0],
            model[0][1], model[1][1], model[2][1], model[3][1],
            model[0][2], model[1][2], model[2][2], model[3][2]
        };

        VkGeometryInstanceFlagsKHR flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;

        if (!meshes[i]->IsOccluder())
            flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;

        instances.emplace_back(transformMatrix, 0, 0xFF, 0, flags, m_BottomLevelASs[i].deviceAddress);
    }

    m_InstanceBuffer.reserve(count);
    m_TopLevelAS.reserve(count);

    for (int i = 0; i < count; i++)
    {
        AccelerationStructure currentTopLevelAS;
        
        m_InstanceBuffer.emplace_back(CreateInstanceBuffer(device, vmaAllocator, instances));

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data = m_InstanceBuffer[i].instanceDataDeviceAddress;

        // Get size info
        /*
        The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
        */
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        uint32_t primitive_count = static_cast<uint32_t>(instances.size());

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device, 
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &primitive_count,
            &accelerationStructureBuildSizesInfo);

        CreateAccelerationStructureBuffer(device, vmaAllocator, currentTopLevelAS, accelerationStructureBuildSizesInfo);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = currentTopLevelAS.buffer;
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &currentTopLevelAS.handle) != VK_SUCCESS)
            std::cout << "Fail to create Acceleration Structure !" << "\n";

        // Create a small scratch buffer used during build of the top level acceleration structure
        RayTracingScratchBuffer scratchBuffer = CreateScratchBuffer(device, vmaAllocator, accelerationStructureBuildSizesInfo.buildScratchSize);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = currentTopLevelAS.handle;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
        accelerationStructureBuildRangeInfo.primitiveCount = primitive_count;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

        VkCommandBuffer commandBuffer = VulkanUtils::BeginSingleTimeCommands(device, computePool);
        
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());

        VulkanUtils::EndSingleTimeCommands(device, computePool, computeQueue, commandBuffer);

        VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
        accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationDeviceAddressInfo.accelerationStructure = currentTopLevelAS.handle;
        currentTopLevelAS.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

        if (currentTopLevelAS.deviceAddress == NULL)
            std::cout << "Fail to get acceleration structure address !" << "\n";

        m_TopLevelAS.emplace_back(currentTopLevelAS);
        
        DeleteScratchBuffer(vmaAllocator, scratchBuffer);
    }
}

void RayTracingAccelerationStructure::CreateAccelerationStructureBuffer(VkDevice device, VmaAllocator allocator, AccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    
    VmaAllocationCreateInfo bufferAllocInfo = {};
    bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    bufferAllocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    if(vmaCreateBuffer(allocator, &bufferCreateInfo, &bufferAllocInfo, &accelerationStructure.buffer, &accelerationStructure.memory, NULL) != VK_SUCCESS)
        std::cout << "Failed to create acceleration structure buffer !" << "\n";
}

RayTracingScratchBuffer RayTracingAccelerationStructure::CreateScratchBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size)
{
    RayTracingScratchBuffer scratchBuffer{};
    
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    
    VmaAllocationCreateInfo bufferAllocInfo = {};
    bufferAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    bufferAllocInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    if(vmaCreateBuffer(allocator, &bufferCreateInfo, &bufferAllocInfo, &scratchBuffer.handle, &scratchBuffer.memory,  &scratchBuffer.memoryInfo) != VK_SUCCESS)
        std::cout << "Failed to create scratch buffer !" << "\n";

    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.handle;
    scratchBuffer.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    if (scratchBuffer.deviceAddress == NULL)
        std::cout << "Fail to get scratch buffer address !" << "\n";

    return scratchBuffer;
}

InstanceBuffer RayTracingAccelerationStructure::CreateInstanceBuffer(VkDevice device, VmaAllocator allocator, std::vector<VkAccelerationStructureInstanceKHR>& instances)
{
    InstanceBuffer instanceBuffer;

    // Taille totale du buffer en octets
    VkDeviceSize bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

    // 1. Créer le buffer avec les usages requis
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                       VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR ;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Spécifier que le buffer doit avoir un addressable device address
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    // Allouer le buffer avec VMA
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &instanceBuffer.buffer, &instanceBuffer.memory, &instanceBuffer.memoryInfo) != VK_SUCCESS)
        std::cout << "Failed to create instance buffer with VMA!" << "\n";

    // 2. Copier les données des instances dans le buffer
    memcpy(instanceBuffer.memoryInfo.pMappedData, instances.data(), (size_t)bufferSize);

    // 3. Obtenir l'adresse GPU du buffer
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = instanceBuffer.buffer;
    instanceBuffer.instanceDataDeviceAddress.deviceAddress = vkGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);

    if (instanceBuffer.instanceDataDeviceAddress.deviceAddress == NULL)
        std::cout << "Failed to get instanceBuffer deviceAddress!" << "\n";

    return instanceBuffer;
}

void RayTracingAccelerationStructure::DeleteScratchBuffer(VmaAllocator allocator, const RayTracingScratchBuffer& scratchBuffer)
{
    if (scratchBuffer.memory != VK_NULL_HANDLE && scratchBuffer.handle != VK_NULL_HANDLE)
        vmaDestroyBuffer(allocator, scratchBuffer.handle, scratchBuffer.memory);
}

void RayTracingAccelerationStructure::CreateDescriptorSets(VkDevice device, VmaAllocator allocator, const std::vector<VkImageView>& imageViews)
{
    VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
    accelerationStructureLayoutBinding.binding = 0;
    accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    accelerationStructureLayoutBinding.descriptorCount = 1;
    accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
    resultImageLayoutBinding.binding = 1;
    resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageLayoutBinding.descriptorCount = 1;
    resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    VkDescriptorSetLayoutBinding uniformBufferBinding{};
    uniformBufferBinding.binding = 2;
    uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferBinding.descriptorCount = 1;
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

    m_TopLevelASDescriptor.CreateDescriptorSetLayout(device, { accelerationStructureLayoutBinding, resultImageLayoutBinding, uniformBufferBinding }, 0);

    for (size_t i = 0; i < imageViews.size(); i++)
        m_TopLevelASDescriptor.AddUniformBuffer(allocator, sizeof(UniformData));

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(3);
    
    VkDescriptorPoolSize descriptorPoolSize;
    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    descriptorPoolSize.descriptorCount = static_cast<uint32_t>(imageViews.size());
    poolSizes.push_back(descriptorPoolSize);

    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes.push_back(descriptorPoolSize);

    descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes.push_back(descriptorPoolSize);
    
    m_TopLevelASDescriptor.CreateDescriptorPool(device, poolSizes, static_cast<uint32_t>(imageViews.size()));

    std::vector<VkDescriptorSetLayout> layouts; 
    layouts.assign(imageViews.size(), m_TopLevelASDescriptor.GetDescriptorSetLayout());

    m_TopLevelASDescriptor.AllocateDescriptorSet(device, layouts);

    auto descriptorSet = m_TopLevelASDescriptor.GetDescriptorSets();
    auto uniformBuffer = m_TopLevelASDescriptor.GetUniformBuffers();

    for (size_t i = 0; i < imageViews.size(); i++)
    {
        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &m_TopLevelAS[i].handle;
        
        std::array<VkWriteDescriptorSet, 3> descriptorSetWrites{};
        descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // The specialized acceleration structure descriptor has to be chained
        descriptorSetWrites[0].pNext = &descriptorAccelerationStructureInfo;
        descriptorSetWrites[0].dstSet = descriptorSet[i];
        descriptorSetWrites[0].dstBinding = 0;
        descriptorSetWrites[0].descriptorCount = 1;
        descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptorImageInfo.imageView = imageViews[i];
        descriptorImageInfo.sampler = VK_NULL_HANDLE;

        descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[1].pNext = NULL;
        descriptorSetWrites[1].dstSet = descriptorSet[i];
        descriptorSetWrites[1].dstBinding = 1;
        descriptorSetWrites[1].descriptorCount = 1;
        descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetWrites[1].pImageInfo = &descriptorImageInfo;

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformData);

        descriptorSetWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[2].pNext = NULL;
        descriptorSetWrites[2].dstSet = descriptorSet[i];
        descriptorSetWrites[2].dstBinding = 2;
        descriptorSetWrites[2].descriptorCount = 1;
        descriptorSetWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetWrites[2].pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorSetWrites.size()), descriptorSetWrites.data(), 0, VK_NULL_HANDLE);
    }
}

void RayTracingAccelerationStructure::CreateRayTracingPipeline(VkDevice device, const std::vector<VkDescriptorSetLayout>& layouts)
{
    std::vector<VkDescriptorSetLayout> rayTracingPipelineLayouts;
    rayTracingPipelineLayouts.reserve(layouts.size() + 1);
    
    rayTracingPipelineLayouts.emplace_back(m_TopLevelASDescriptor.GetDescriptorSetLayout());
    for (const auto& layout : layouts)
        rayTracingPipelineLayouts.emplace_back(layout);
    
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(rayTracingPipelineLayouts.size());
    pipelineLayoutCI.pSetLayouts = rayTracingPipelineLayouts.data();
    vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &m_RayTracingPipelineLayout);

    Shader raygenShader;
    raygenShader.createModule(device, ".\\Shader\\raygen.spv");
    VkPipelineShaderStageCreateInfo raygenShaderStageCreateInfo;
    raygenShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    raygenShaderStageCreateInfo.pNext = NULL;
    raygenShaderStageCreateInfo.flags = 0;
    raygenShaderStageCreateInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    raygenShaderStageCreateInfo.module = raygenShader.getShaderModule();
    raygenShaderStageCreateInfo.pName = "main";
    raygenShaderStageCreateInfo.pSpecializationInfo = NULL;
    
    Shader missShader;
    missShader.createModule(device, ".\\Shader\\miss.spv");
    VkPipelineShaderStageCreateInfo missShaderStageCreateInfo;
    missShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    missShaderStageCreateInfo.pNext = NULL;
    missShaderStageCreateInfo.flags = 0;
    missShaderStageCreateInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    missShaderStageCreateInfo.module = missShader.getShaderModule();
    missShaderStageCreateInfo.pName = "main";
    missShaderStageCreateInfo.pSpecializationInfo = NULL;

    Shader closestHitShader;
    closestHitShader.createModule(device, ".\\Shader\\closesthit.spv");
    VkPipelineShaderStageCreateInfo closestHitShaderStageCreateInfo;
    closestHitShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    closestHitShaderStageCreateInfo.pNext = NULL;
    closestHitShaderStageCreateInfo.flags = 0;
    closestHitShaderStageCreateInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    closestHitShaderStageCreateInfo.module = closestHitShader.getShaderModule();
    closestHitShaderStageCreateInfo.pName = "main";
    closestHitShaderStageCreateInfo.pSpecializationInfo = NULL;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		// Ray generation group
		{
			shaderStages.push_back(raygenShaderStageCreateInfo);
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroup);
		}

		// Miss group
		{
			shaderStages.push_back(missShaderStageCreateInfo);
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroup);
		}

		// Closest hit group
		{
			shaderStages.push_back(closestHitShaderStageCreateInfo);
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			m_ShaderGroups.push_back(shaderGroup);
		}

    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
    rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayTracingPipelineCI.pStages = shaderStages.data();
    rayTracingPipelineCI.groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
    rayTracingPipelineCI.pGroups = m_ShaderGroups.data();
    rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
    rayTracingPipelineCI.layout = m_RayTracingPipelineLayout;

    vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &m_RayTracingPipeline);

    raygenShader.cleanup(device);
    missShader.cleanup(device);
    closestHitShader.cleanup(device);
}

void RayTracingAccelerationStructure::CreateShaderBindingTable(VkDevice device, VmaAllocator allocator)
{
    const uint32_t handleSize = m_RayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = VulkanUtils::alignedSize(m_RayTracingPipelineProperties.shaderGroupHandleSize, m_RayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
    const uint32_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    vkGetRayTracingShaderGroupHandlesKHR(device, m_RayTracingPipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = handleSize;
    bufferInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    // Allouer le buffer avec VMA
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_RayGenShaderBindingTable.buffer, &m_RayGenShaderBindingTable.memory, nullptr) != VK_SUCCESS)
        std::cout << "Failed to create RayGenShaderBindingTable buffer with VMA!" << "\n";

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_MissShaderBindingTable.buffer, &m_MissShaderBindingTable.memory, nullptr) != VK_SUCCESS)
        std::cout << "Failed to create MissShaderBindingTable buffer with VMA!" << "\n";

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_HitShaderBindingTable.buffer, &m_HitShaderBindingTable.memory, nullptr) != VK_SUCCESS)
        std::cout << "Failed to create HitShaderBindingTable buffer with VMA!" << "\n";

    void* data;
    vmaMapMemory(allocator, m_RayGenShaderBindingTable.memory, &data);
    memcpy(data, shaderHandleStorage.data(), handleSize);
    vmaUnmapMemory(allocator, m_RayGenShaderBindingTable.memory);
    
    vmaMapMemory(allocator, m_MissShaderBindingTable.memory, &data);
    memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize);
    vmaUnmapMemory(allocator, m_MissShaderBindingTable.memory);
    
    vmaMapMemory(allocator, m_HitShaderBindingTable.memory, &data);
    memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
    vmaUnmapMemory(allocator, m_HitShaderBindingTable.memory);

    VkBufferDeviceAddressInfo deviceAddressInfo;
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.pNext = NULL;
    deviceAddressInfo.buffer = m_RayGenShaderBindingTable.buffer;
    m_RayGenShaderBindingTable.deviceAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

    deviceAddressInfo.buffer = m_MissShaderBindingTable.buffer;
    m_MissShaderBindingTable.deviceAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

    deviceAddressInfo.buffer = m_HitShaderBindingTable.buffer;
    m_HitShaderBindingTable.deviceAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);
}

void RayTracingAccelerationStructure::RecordCmdTraceRay(VkDevice device, VmaAllocator vmaAllocator, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t width, uint32_t height)
{
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = m_InstanceBuffer[imageIndex].instanceDataDeviceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
    buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
    buildGeometryInfo.dstAccelerationStructure = m_TopLevelAS[imageIndex].handle;
    buildGeometryInfo.srcAccelerationStructure = m_TopLevelAS[imageIndex].handle;
    buildGeometryInfo.geometryCount = 1;
    buildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    uint32_t primitiveCount = static_cast<uint32_t>(m_BottomLevelASs.size());

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        device, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    RayTracingScratchBuffer& scratchBuffer = m_UpdateScratchBuffers[imageIndex];

    if (scratchBuffer.handle == VK_NULL_HANDLE)
    {
        scratchBuffer = CreateScratchBuffer(device,  vmaAllocator, accelerationStructureBuildSizesInfo.updateScratchSize);
    }
    else if (scratchBuffer.memoryInfo.size < accelerationStructureBuildSizesInfo.updateScratchSize)
    {
        DeleteScratchBuffer(vmaAllocator, scratchBuffer);
        scratchBuffer.handle = VK_NULL_HANDLE;

        scratchBuffer = CreateScratchBuffer(device,  vmaAllocator, accelerationStructureBuildSizesInfo.updateScratchSize);
    }
    
    buildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = primitiveCount;
    VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos[] = { &buildRangeInfo };
    
    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo, pBuildRangeInfos);
    
    const uint32_t handleSizeAligned = VulkanUtils::alignedSize(m_RayTracingPipelineProperties.shaderGroupHandleSize, m_RayTracingPipelineProperties.shaderGroupHandleAlignment);

    VkBufferDeviceAddressInfo deviceAddressInfo;
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.pNext = NULL;
    deviceAddressInfo.buffer = m_RayGenShaderBindingTable.buffer;

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = m_RayGenShaderBindingTable.deviceAddress;
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = m_MissShaderBindingTable.deviceAddress;
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = m_HitShaderBindingTable.deviceAddress;
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    VkMemoryBarrier memoryBarrier = {};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.pNext = NULL;
    memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,  // Source stage
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,            // Destination stage
        0,                                                      // No dependency flags
        1,                                                      // Memory barrier count
        &memoryBarrier,                                          // Memory barrier
        0, NULL,                                                // No buffer barriers
        0, NULL                                                 // No image barriers
    );

    vkCmdTraceRaysKHR(
        commandBuffer,
        &raygenShaderSbtEntry,
        &missShaderSbtEntry,
        &hitShaderSbtEntry,
        &callableShaderSbtEntry,
        width,
        height,
        1);
}

void RayTracingAccelerationStructure::UpdateUniform(const glm::mat4& viewInverse, const glm::mat4& projInverse, uint32_t imageIndex, const std::vector<Mesh*>& meshes)
{
    auto uniformAllocInfo = m_TopLevelASDescriptor.GetUniformAllocationInfos()[imageIndex];

    UniformData uniformData;
    uniformData.viewInverse = viewInverse;
    uniformData.projInverse = projInverse;

    memcpy(uniformAllocInfo.pMappedData, &uniformData, sizeof(UniformData));
}

void RayTracingAccelerationStructure::UpdateTransform(uint32_t imageIndex, const std::vector<uint32_t>& transformIndexs, const std::vector<glm::mat4>& transforms, const std::vector<bool>& nonOccluderIndexs)
{
    for (int i = 0; i < transformIndexs.size(); i++)
    {
        uint32_t transformIndex = transformIndexs[i];
        glm::mat4 transform = transforms[i];
        
        VkTransformMatrixKHR transformMatrix = {
            transform[0][0], transform[1][0], transform[2][0], transform[3][0],
            transform[0][1], transform[1][1], transform[2][1], transform[3][1],
            transform[0][2], transform[1][2], transform[2][2], transform[3][2]
        };

        VkAccelerationStructureInstanceKHR instance;
        instance.transform = transformMatrix;
        instance.instanceCustomIndex = 0;
        instance.mask = 0xFF;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.accelerationStructureReference = m_BottomLevelASs[transformIndex].deviceAddress;

        bool nonOccluder = false;
        
        for (auto nonOccluderIndex : nonOccluderIndexs)
        {
            nonOccluder = nonOccluder || (i == nonOccluderIndex);
            break;
        }

        VkGeometryInstanceFlagsKHR flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;

        if (nonOccluder)
            flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;

        instance.flags = flags;
        
        memcpy(reinterpret_cast<uint8_t*>(m_InstanceBuffer[imageIndex].memoryInfo.pMappedData) + transformIndex * sizeof(VkAccelerationStructureInstanceKHR), &instance, sizeof(VkAccelerationStructureInstanceKHR));
    }
}

void RayTracingAccelerationStructure::UpdateImageDescriptor(VkDevice device, const std::vector<VkImageView>& imageViews)
{
    std::vector<VkDescriptorSet> descriptorSet = m_TopLevelASDescriptor.GetDescriptorSets();
    
    for (size_t i = 0; i < imageViews.size(); i++)
    {
        VkWriteDescriptorSet descriptorSetWrite{};

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        descriptorImageInfo.imageView = imageViews[i];
        descriptorImageInfo.sampler = VK_NULL_HANDLE;

        descriptorSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrite.pNext = NULL;
        descriptorSetWrite.dstSet = descriptorSet[i];
        descriptorSetWrite.dstBinding = 1;
        descriptorSetWrite.descriptorCount = 1;
        descriptorSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorSetWrite.pImageInfo = &descriptorImageInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorSetWrite, 0, VK_NULL_HANDLE);
    }
}

VkPipelineLayout RayTracingAccelerationStructure::GetRayTracingPipelineLayout()
{
    return m_RayTracingPipelineLayout;
}

bool* RayTracingAccelerationStructure::GetActiveRaytracingPtr()
{
    return &m_ActiveRayTracing;
}
