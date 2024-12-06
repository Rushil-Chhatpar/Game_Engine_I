#pragma once
#include <vector>

#include "Vertex.h"

class Mesh
{
public:
	Mesh()
	{}
	// move constructor
	Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
		: _vertices(std::move(vertices))
		, _indices(std::move(indices))
	{}

	const std::vector<Vertex>& GetVertices() const { return _vertices; }
	const size_t& GetVerticesSize() const { return _vertices.size(); }
	const std::vector<uint32_t>& GetIndices() const { return _indices; }
	const size_t& GetIndicesSize() const { return _indices.size(); }

private:
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
};
