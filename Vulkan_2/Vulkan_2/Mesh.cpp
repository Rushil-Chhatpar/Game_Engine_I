#include "pch.h"
#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
	: _vertices(std::move(vertices))
	, _indices(std::move(indices))
{
}
