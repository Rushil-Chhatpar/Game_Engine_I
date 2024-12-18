#include "pch.h"
#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>

Mesh::Mesh()
{
}

Mesh::Mesh(const char* file)
{
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
}

Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
	: _vertices(std::move(vertices))
	, _indices(std::move(indices))
{
}
