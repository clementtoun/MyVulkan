#pragma once

#include "Mesh.h"
#include "VulkanBase.h"
#include "VulkanUtils.h"
#include "Descriptor.h"
#include "Shader.h"

struct InstanceBuffer
{
    VmaAllocation memory;
    VkBuffer buffer;
    VmaAllocationInfo memoryInfo;
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress;
};

struct RayTracingScratchBuffer
{
    uint64_t deviceAddress = 0;
    VkBuffer handle = VK_NULL_HANDLE;
    VmaAllocation memory = VK_NULL_HANDLE;
};

struct AccelerationStructure {
    VkAccelerationStructureKHR handle;
    uint64_t deviceAddress = 0;
    VmaAllocation memory;
    VkBuffer buffer;
};

struct ShaderBindingTable {
    VmaAllocation memory;
    VkBuffer buffer;
    VkDeviceAddress deviceAddress;
};

struct UniformData {
    glm::mat4 viewInverse;
    glm::mat4 projInverse;
};

class RayTracingAccelerationStructure
{
public:

    RayTracingAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes, const std::vector<VkImageView>& imageViews);

    void RecordCommandBuffer(VkDevice device, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t width, uint32_t height);

    void UpdateUniform(const glm::mat4& viewInverse, const glm::mat4& projInverse, uint32_t imageIndex, const std::vector<Mesh*>& meshes);

    void UpdateTransform(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, uint32_t transformIndex, const glm::mat4& transform);

    void UpdateImageDescriptor(VkDevice device, const std::vector<VkImageView>& imageViews);

    bool* GetActiveRaytracingPtr();
    
    void Cleanup(VkDevice device, VmaAllocator vmaAllocator);

private:

    void CreateBottomLevelASs(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes);

    void CreateTopLevelAS(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes);

    void CreateAccelerationStructureBuffer(VkDevice device, VmaAllocator allocator, AccelerationStructure &accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

    RayTracingScratchBuffer CreateScratchBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size);

    InstanceBuffer CreateInstanceBuffer(VkDevice device, VmaAllocator allocator, std::vector<VkAccelerationStructureInstanceKHR>& instances);

    void DeleteScratchBuffer(VmaAllocator allocator, const RayTracingScratchBuffer& scratchBuffer);

    void CreateDescriptorSets(VkDevice device, VmaAllocator allocator, const std::vector<VkImageView>& imageViews);

    void CreateRayTracingPipeline(VkDevice device);

    void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator);

    bool m_ActiveRayTracing = 0;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  m_RayTracingPipelineProperties;
    
    std::vector<AccelerationStructure> m_BottomLevelASs{};
    AccelerationStructure m_TopLevelAS{};

    InstanceBuffer m_InstanceBuffer;
    Descriptor m_TopLevelASDescriptor;

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups{};

    VkPipeline m_RayTracingPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_RayTracingPipelineLayout = VK_NULL_HANDLE;

    ShaderBindingTable m_RayGenShaderBindingTable;
    ShaderBindingTable m_MissShaderBindingTable;
    ShaderBindingTable m_HitShaderBindingTable;
    
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;
};
