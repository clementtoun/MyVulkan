#include "Mesh.h"
#include "VulkanUtils.h"
#include <iostream>
#include <list>

Mesh::Mesh()
{
	m_Model = glm::mat4(1.);
}

Mesh::Mesh(const std::vector<Vertex>& vertexs, const std::vector<uint32_t>& indexes)
{
	m_Vertexs = vertexs;
	m_Indexes = indexes;
	m_Model = glm::mat4(1.);
}

void Mesh::DestroyBuffer(VmaAllocator allocator, VkDevice device)
{
	if (m_VertexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, m_VertexBuffer, NULL);
		vmaFreeMemory(allocator, m_VertexBufferAlloc);
	}

	if (m_IndexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, m_IndexBuffer, NULL);
		vmaFreeMemory(allocator, m_IndexBufferAlloc);
	}
}

void Mesh::SetVertex(const std::vector<Vertex>& vertexs)
{
	m_Vertexs.clear();
	m_Vertexs = vertexs;
}

const std::vector<Vertex>& Mesh::GetVertex()
{
	return m_Vertexs;
}

void Mesh::SetIndexes(const std::vector<uint32_t>& indexes)
{
	m_Indexes.clear();
	m_Indexes = indexes;
}

const std::vector<uint32_t>& Mesh::GetIndexes()
{
	return m_Indexes;
}

void Mesh::AddVertex(const Vertex& vertex)
{
	m_Vertexs.push_back(vertex);
}

void Mesh::AddIndex(const uint32_t& index)
{
	m_Indexes.push_back(index);
}

size_t Mesh::AddPrimitives(const Primitive& primitive)
{
	m_Primitves.push_back(primitive);

	return m_Primitves.size() - 1;
}

const std::vector<Primitive>& Mesh::GetPrimitives()
{
	return m_Primitves;
}  

void Mesh::CreateVertexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice)
{
	uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

	VkDeviceSize vertexBufferSize = m_Vertexs.size() * sizeof(Vertex);

	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = vertexBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	stagingBufferInfo.queueFamilyIndexCount = 2;
	stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuf = {};
	VmaAllocation stagingAlloc = {};
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

	void* data = nullptr;
	vmaMapMemory(allocator, stagingAlloc, &data);
	memcpy(data, m_Vertexs.data(), (size_t)vertexBufferSize);
	vmaUnmapMemory(allocator, stagingAlloc);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = vertexBufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocCreateInfo.priority = 1.0f;

	vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_VertexBuffer, &m_VertexBufferAlloc, NULL);

	VulkanUtils::CopyBuffer(device, transferPool, transferQueue, stagingBuf, m_VertexBuffer, vertexBufferSize);

	vkDestroyBuffer(device, stagingBuf, nullptr);
	vmaFreeMemory(allocator, stagingAlloc);
}

void Mesh::CreateIndexBuffers(VmaAllocator allocator, VkDevice device, VkCommandPool transferPool, VkQueue transferQueue, uint32_t transferFamilyIndice, uint32_t graphicFamilyIndice)
{
	uint32_t queueFamilyIndices[2] = { transferFamilyIndice, graphicFamilyIndice };

	VkDeviceSize indexBufferSize = m_Indexes.size() * sizeof(uint32_t);

	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = indexBufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	stagingBufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	stagingBufferInfo.queueFamilyIndexCount = 2;
	stagingBufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo stagingAllocInfo = {};
	stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
	stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuf = {};
	VmaAllocation stagingAlloc = {};

	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuf, &stagingAlloc, NULL);

	void* data = nullptr;
	vmaMapMemory(allocator, stagingAlloc, &data);
	memcpy(data, m_Indexes.data(), (size_t)indexBufferSize);
	vmaUnmapMemory(allocator, stagingAlloc);

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = indexBufferSize;
	bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	bufferInfo.queueFamilyIndexCount = 2;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocCreateInfo.priority = 1.0f;

	vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &m_IndexBuffer, &m_IndexBufferAlloc, NULL);

	VulkanUtils::CopyBuffer(device, transferPool, transferQueue, stagingBuf, m_IndexBuffer, indexBufferSize);

	vkDestroyBuffer(device, stagingBuf, nullptr);
	vmaFreeMemory(allocator, stagingAlloc);
}

void Mesh::BindVertexBuffer(VkCommandBuffer commandBuffer)
{
	VkBuffer vertexBuffers[] = { m_VertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

void Mesh::BindIndexBuffer(VkCommandBuffer commandBuffer)
{
	vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
}

bool Mesh::GetAccelerationStructureGeometrys(VkDevice device, std::vector<VkAccelerationStructureGeometryKHR>& accelerationStructureGeometrys)
{
	VkBufferDeviceAddressInfo deviceAddressInfo;
	deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	deviceAddressInfo.pNext = NULL;
	
	deviceAddressInfo.buffer = m_VertexBuffer;
	uint64_t vertexDeviceAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);
	
	deviceAddressInfo.buffer = m_IndexBuffer;
	uint64_t indexDeviceAddress = vkGetBufferDeviceAddress(device, &deviceAddressInfo);

	if (vertexDeviceAddress == NULL || indexDeviceAddress == NULL)
	{
		std::cout << "Failed to get vertexBufferDeviceAddress or indexBufferDeviceAddress ! " << "\n";
		return false;
	}

	accelerationStructureGeometrys.reserve(m_Primitves.size());
	
	for(Primitive& primitive : m_Primitves)
	{
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.pNext = NULL;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexDeviceAddress;
		accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		accelerationStructureGeometry.geometry.triangles.maxVertex = primitive.vertexCount - 1;
		accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
		accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;

		accelerationStructureGeometrys.emplace_back(accelerationStructureGeometry);
	}

	return true;
}

void Mesh::GetAccelerationStructureRangeInfos(std::vector<VkAccelerationStructureBuildRangeInfoKHR>& accelerationStructureRangeInfos)
{
	accelerationStructureRangeInfos.reserve(m_Primitves.size());
	
	for(int i = 0; i < m_Primitves.size(); i++)
	{
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
		rangeInfo.primitiveCount = m_Primitves[i].indexCount / 3;
		rangeInfo.primitiveOffset = m_Primitves[i].firstIndex * sizeof(uint32_t);
		rangeInfo.firstVertex = m_Primitves[i].vertexOffset;
		rangeInfo.transformOffset = 0;

		accelerationStructureRangeInfos.emplace_back(rangeInfo);
	}
}

void Mesh::GetPrimitvesTrianglesCounts(std::vector<uint32_t>& primitivesTrianglesCounts)
{
	primitivesTrianglesCounts.reserve(m_Primitves.size());

	for(Primitive& primitive : m_Primitves)
		primitivesTrianglesCounts.emplace_back(primitive.indexCount / 3);
}

const glm::mat4& Mesh::GetModel()
{
	return m_Model;
}

void Mesh::SetModel(const glm::mat4& model)
{
	m_Model = model;
}

void Mesh::AutoComputeNormalsPrimitive(size_t primitiveIndex)
{
	Primitive& p = m_Primitves[primitiveIndex];

	uint32_t lastPrimitiveVertexIdx = p.vertexOffset + p.vertexCount;

	for (uint32_t i = p.vertexOffset; i < lastPrimitiveVertexIdx; i++)
		m_Vertexs[i].normal = glm::vec3(0.);

	for (uint32_t i = p.firstIndex; i < p.firstIndex + p.indexCount; i += 3)
	{
		uint32_t idx0 = m_Indexes[i] + p.vertexOffset;
		uint32_t idx1 = m_Indexes[i + 1] + p.vertexOffset;
		uint32_t idx2 = m_Indexes[i + 2] + p.vertexOffset;

		glm::vec3 normal = glm::normalize(glm::cross(m_Vertexs[idx1].pos - m_Vertexs[idx0].pos, m_Vertexs[idx2].pos - m_Vertexs[idx0].pos));

		m_Vertexs[idx0].normal += normal;
		m_Vertexs[idx1].normal += normal;
		m_Vertexs[idx2].normal += normal;
	}

	for (uint32_t i = p.vertexOffset; i < lastPrimitiveVertexIdx; i++)
		m_Vertexs[i].normal = glm::normalize(m_Vertexs[i].normal);
}

void Mesh::AutoComputeTangentsBiTangentsPrimitive(size_t primitiveIndex)
{
	Primitive& p = m_Primitves[primitiveIndex];

	uint32_t lastPrimitiveVertexIdx = p.vertexOffset + p.vertexCount;

	for (uint32_t i = p.vertexOffset; i < lastPrimitiveVertexIdx; i++)
		m_Vertexs[i].tangent = glm::vec3(0.F);

	for (uint32_t i = p.firstIndex; i < p.firstIndex + p.indexCount; i += 3)
	{
		uint32_t idx0 = m_Indexes[i] + p.vertexOffset;
		uint32_t idx1 = m_Indexes[i + 1] + p.vertexOffset;
		uint32_t idx2 = m_Indexes[i + 2] + p.vertexOffset;

		glm::vec3 E1 = m_Vertexs[idx1].pos - m_Vertexs[idx0].pos;
		glm::vec3 E2 = m_Vertexs[idx2].pos - m_Vertexs[idx0].pos;

		float u10 = m_Vertexs[idx1].tex_coord.x - m_Vertexs[idx0].tex_coord.x;
		float v10 = m_Vertexs[idx1].tex_coord.y - m_Vertexs[idx0].tex_coord.y;
		float u20 = m_Vertexs[idx2].tex_coord.x - m_Vertexs[idx0].tex_coord.x;
		float v20 = m_Vertexs[idx2].tex_coord.y - m_Vertexs[idx0].tex_coord.y;

		float denom = u10 * v20 - v10 * u20;

		glm::vec3 tangent = glm::vec3(1.F, 0.F, 0.F);
		glm::vec3 biTangent = glm::vec3(0.F, 1.F, 0.F);

		if (denom != 0.F)
		{
			tangent = (v20 * E1 - v10 * E2) / denom;
			biTangent = (u10 * E2 - u20 * E1) / denom;
		}

		m_Vertexs[idx0].tangent += tangent;
		m_Vertexs[idx1].tangent += tangent;
		m_Vertexs[idx2].tangent += tangent;

		m_Vertexs[idx0].biTangent += biTangent;
		m_Vertexs[idx1].biTangent += biTangent;
		m_Vertexs[idx2].biTangent += biTangent;
	}

	for (uint32_t i = p.vertexOffset; i < lastPrimitiveVertexIdx; i++)
	{
		const glm::vec3 normal = m_Vertexs[i].normal;
		glm::vec3& tangent = m_Vertexs[i].tangent;
		glm::vec3& biTangent = m_Vertexs[i].biTangent;

		tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);

		biTangent = glm::cross(normal, tangent) * (glm::dot(glm::cross(normal, tangent), biTangent) < 0.F ? -1.F : 1.F);
	}
}

void Mesh::AutoComputeBiTangentsPrimitive(size_t primitiveIndex)
{
	Primitive& p = m_Primitves[primitiveIndex];

	for (uint32_t i = p.vertexOffset; i < p.vertexOffset + p.vertexCount; i++)
	{
		const glm::vec3 normal = m_Vertexs[i].normal;
		const glm::vec3 tangent = m_Vertexs[i].tangent;
		m_Vertexs[i].tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);

		m_Vertexs[i].biTangent = glm::cross(m_Vertexs[i].normal, m_Vertexs[i].tangent);
	}
}

void Mesh::AutoComputeNormals()
{
	for (size_t i = 0; i < m_Primitves.size(); i++)
		AutoComputeNormalsPrimitive(i);
}

void Mesh::AutoComputeTangentsBiTangents()
{
	for (size_t i = 0; i < m_Primitves.size(); i++)
		AutoComputeTangentsBiTangentsPrimitive(i);
}

void Mesh::AutoComputeBiTangents()
{
	for (size_t i = 0; i < m_Primitves.size(); i++)
		AutoComputeBiTangentsPrimitive(i);
}

void Mesh::AverageDuplicatedVertexNormals()
{
	struct posIndex {
		glm::vec3 pos;
		size_t index;
	};

	std::list<struct posIndex> posIndexList;

	for (size_t i = 0; i < m_Vertexs.size(); i++)
		posIndexList.push_back(struct posIndex(m_Vertexs[i].pos, i));

	std::vector<std::vector<size_t>> duplicatedIndexes;
	
	for (auto iIt = posIndexList.begin(); iIt != std::prev(posIndexList.end()); ++iIt)
	{
		std::vector<size_t> currentDuplicatedIndexes;
		bool isDuplicated = false;
		auto ipos = iIt->pos;

		for (auto jIt = std::next(iIt); jIt != posIndexList.end(); ++jIt)
		{
			auto jpos = jIt->pos;

			constexpr float epsilon = std::numeric_limits<float>::epsilon();

			if (fabs(ipos.x - jpos.x) <= epsilon && fabs(ipos.y - jpos.y) <= epsilon && fabs(ipos.z - jpos.z) <= epsilon)
			{
				currentDuplicatedIndexes.push_back(jIt->index);
				jIt = std::prev(posIndexList.erase(jIt));

				if (!isDuplicated)
				{
					currentDuplicatedIndexes.push_back(iIt->index);
					isDuplicated = true;
				}
			}

		}

		if (isDuplicated)
			duplicatedIndexes.push_back(currentDuplicatedIndexes);
	}

	for (auto& duplicatedIndexesGroup : duplicatedIndexes)
	{
		glm::vec3 normalAcc = glm::vec3(0.);

		for (auto duplicatedIndexe : duplicatedIndexesGroup)
			normalAcc += m_Vertexs[duplicatedIndexe].normal;

		normalAcc = glm::normalize(normalAcc);

		for (auto duplicatedIndexe : duplicatedIndexesGroup)
			m_Vertexs[duplicatedIndexe].normal = normalAcc;
	}
}



