#pragma once
#include <vector>

#include "Vertex.h"

class Mesh
{
public:
	Mesh()
	{}
	// move constructor
	Mesh(std::vector<Vertex>&& vertices)
		: _vertices(std::move(vertices))
	{}

	const std::vector<Vertex>& GetVertices() const { return _vertices; }

private:
	std::vector<Vertex> _vertices;
};
