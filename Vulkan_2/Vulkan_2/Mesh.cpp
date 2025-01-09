#include "pch.h"
#include "Mesh.h"
#include "Buffer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

#include "Application.h"

Resource::Mesh::Mesh()
{
}

Resource::Mesh::Mesh(const char* file)
{
	// Read OBJ file
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, file))
	{
		throw std::runtime_error("Failed to load obj!!!   " + warning + error);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	for (tinyobj::shape_t& shape : shapes)
	{
		for (auto index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2],
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
			};

			vertex.color = {1.0f, 1.0f, 1.0f};

			if(uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
				_vertices.push_back(vertex);
			}
			_indices.push_back(uniqueVertices[vertex]);
		}
	}

	InitializeBuffer();
}

Resource::Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
	: _vertices(std::move(vertices))
	, _indices(std::move(indices))
{
}

Resource::Mesh::~Mesh()
{
	delete _dataBuffer;
}

void Resource::Mesh::InitializeBuffer()
{
	VkDeviceSize verticesSize = sizeof(_vertices.at(0)) * _vertices.size();
	VkDeviceSize indicesSize = sizeof(_indices.at(0)) * _indices.size();
	VkDeviceSize bufferSize = indicesSize + verticesSize;

	_dataBuffer = new Engine::Buffer();

	_dataBuffer->SetVertexOffset(0);
	_dataBuffer->SetIndexOffset(sizeof(_vertices.at(0)) * _vertices.size());

	Engine::Buffer* stagingBuffer = new Engine::Buffer();
	std::vector<uint32_t> transferOpsQueueFamilyIndices = Application::GetTransferOpsQueueIndices();
	stagingBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_CONCURRENT, 2, transferOpsQueueFamilyIndices.data());
	void* data;
	vkMapMemory(Application::s_logicalDevice, stagingBuffer->GetBufferMemory(), 0, bufferSize, 0, &data);
	// copy vertices
	memcpy(static_cast<char*>(data) + _dataBuffer->GetVertexOffset(), _vertices.data(), verticesSize);
	// copy indices
	memcpy(static_cast<char*>(data) + _dataBuffer->GetIndexOffset(), _indices.data(), indicesSize);
	vkUnmapMemory(Application::s_logicalDevice, stagingBuffer->GetBufferMemory());

	_dataBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);

	Application::CopyBuffer(stagingBuffer->GetBuffer(), _dataBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}
