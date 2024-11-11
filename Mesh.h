#pragma once

#include <array>
#include <vector>
#include <limits>
#include <algorithm>
#include "VulkanBase.h"
#include "VkGLM.h"

struct Primitive
{
	uint32_t firstIndex;
	uint32_t indexCount;
	uint32_t vertexOffset;
	uint32_t vertexCount;
	size_t materialID;
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 biTangent;
	glm::vec2 tex_coord;
	uint8_t color[3];

	static VkVertexInputBindingDescription getVertexInputBindingDescription()
	{
		VkVertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = 0;
		vertexInputBindingDescription.stride = sizeof(Vertex);
		vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertexInputBindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 6> getVertexInputAttributeDescription()
	{
		std::array<VkVertexInputAttributeDescription, 6> vertexInputAttributeDescription;
		vertexInputAttributeDescription[0].location = 0;
		vertexInputAttributeDescription[0].binding = 0;
		vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[0].offset = offsetof(Vertex, pos);
		vertexInputAttributeDescription[1].location = 1;
		vertexInputAttributeDescription[1].binding = 0;
		vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[1].offset = offsetof(Vertex, normal);
		vertexInputAttributeDescription[2].location = 2;
		vertexInputAttributeDescription[2].binding = 0;
		vertexInputAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[2].offset = offsetof(Vertex, tangent);
		vertexInputAttributeDescription[3].location = 3;
		vertexInputAttributeDescription[3].binding = 0;
		vertexInputAttributeDescription[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertexInputAttributeDescription[3].offset = offsetof(Vertex, biTangent);
		vertexInputAttributeDescription[4].location = 4;
		vertexInputAttributeDescription[4].binding = 0;
		vertexInputAttributeDescription[4].format = VK_FORMAT_R32G32_SFLOAT;
		vertexInputAttributeDescription[4].offset = offsetof(Vertex, tex_coord);
		vertexInputAttributeDescription[5].location = 5;
		vertexInputAttributeDescription[5].binding = 0;
		vertexInputAttributeDescription[5].format = VK_FORMAT_R8G8B8_UNORM;
		vertexInputAttributeDescription[5].offset = offsetof(Vertex, color);

		return vertexInputAttributeDescription;
	}
};

class Mesh
{
public:
	Mesh();

	Mesh(const std::vector<Vertex>& vertexs, const std::vector<uint32_t>& indexes);

	void DestroyBuffer(VmaAllocator allocator, VkDevice device);

	void SetVertex(const std::vector<Vertex>& vertexs);
	const std::vector<Vertex>& GetVertex();

	void SetIndexes(const std::vector<uint32_t>& indexes);
	const std::vector<uint32_t>& GetIndexes();

	void AddVertex(const Vertex& vertex);

	void AddIndex(const uint32_t& index);

	size_t AddPrimitives(const Primitive& primitive);

	const std::vector<Primitive>& GetPrimitives();

	void CreateVertexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void CreateIndexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice);

	void BindVertexBuffer(VkCommandBuffer commandBuffer);

	void BindIndexBuffer(VkCommandBuffer commandBuffer);

	bool GetAccelerationStructureGeometrys(VkDevice device, std::vector<VkAccelerationStructureGeometryKHR>& accelerationStructureGeometrys);

	void GetAccelerationStructureRangeInfos(std::vector<VkAccelerationStructureBuildRangeInfoKHR>& accelerationStructureRangeInfos);

	void GetPrimitvesTrianglesCounts(std::vector<uint32_t>& primitivesTrianglesCounts);

	const glm::mat4& GetModel();

	void SetModel(const glm::mat4& model);

	void AutoComputeNormalsPrimitive(size_t primitiveIndex);

	void AutoComputeTangentsBiTangentsPrimitive(size_t primitiveIndex);

	void AutoComputeBiTangentsPrimitive(size_t primitiveIndex);

	void AutoComputeNormals();

	void AutoComputeTangentsBiTangents();

	void AutoComputeBiTangents();

	void AverageDuplicatedVertexNormals();

private:

	std::vector<Vertex> m_Vertexs;
	std::vector<uint32_t> m_Indexes;

	std::vector<Primitive> m_Primitves;

	glm::mat4 m_Model;

	VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_VertexBufferAlloc = nullptr;
	VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
	VmaAllocation m_IndexBufferAlloc = nullptr;
};

