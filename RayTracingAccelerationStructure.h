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
    VmaAllocationInfo memoryInfo;
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

struct alignas(16) UniformData {
    alignas(16) glm::mat4 viewInverse;
    alignas(16) glm::mat4 projInverse;
};

class RayTracingAccelerationStructure
{
public:

    RayTracingAccelerationStructure(VkDevice device, VkPhysicalDevice physicalDevice, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes, const std::vector<VkImageView>& imageViews, const std::vector<VkDescriptorSetLayout>& layouts);

    void BindPipeline(VkCommandBuffer commandBuffer);

    void BindTopLevelASDescriptorSet(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    
    void RecordCmdTraceRay(VkDevice device, VmaAllocator vmaAllocator, VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t width, uint32_t height);

    void UpdateUniform(const glm::mat4& viewInverse, const glm::mat4& projInverse, uint32_t imageIndex, const std::vector<Mesh*>& meshes);

    void UpdateTransform(uint32_t imageIndex, const std::vector<uint32_t>& transformIndexs, const std::vector<glm::mat4>& transforms);

    void UpdateImageDescriptor(VkDevice device, const std::vector<VkImageView>& imageViews);

    VkPipelineLayout GetRayTracingPipelineLayout();

    bool* GetActiveRaytracingPtr();
    
    void Cleanup(VkDevice device, VmaAllocator vmaAllocator);

private:

    void CreateBottomLevelASs(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes);

    void CreateTopLevelAS(VkDevice device, VmaAllocator vmaAllocator, VkQueue computeQueue, VkCommandPool computePool, const std::vector<Mesh*>& meshes, int count = 1);

    void CreateAccelerationStructureBuffer(VkDevice device, VmaAllocator allocator, AccelerationStructure &accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

    RayTracingScratchBuffer CreateScratchBuffer(VkDevice device, VmaAllocator allocator, VkDeviceSize size);

    InstanceBuffer CreateInstanceBuffer(VkDevice device, VmaAllocator allocator, std::vector<VkAccelerationStructureInstanceKHR>& instances);

    void DeleteScratchBuffer(VmaAllocator allocator, const RayTracingScratchBuffer& scratchBuffer);

    void CreateDescriptorSets(VkDevice device, VmaAllocator allocator, const std::vector<VkImageView>& imageViews);

    void CreateRayTracingPipeline(VkDevice device, const std::vector<VkDescriptorSetLayout>& layouts);

    void CreateShaderBindingTable(VkDevice device, VmaAllocator allocator);

    bool m_ActiveRayTracing = 0;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR  m_RayTracingPipelineProperties;
    
    std::vector<AccelerationStructure> m_BottomLevelASs{};
    std::vector<AccelerationStructure> m_TopLevelAS{};
    std::vector<InstanceBuffer> m_InstanceBuffer;
    std::vector<RayTracingScratchBuffer> m_UpdateScratchBuffers;
    
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
