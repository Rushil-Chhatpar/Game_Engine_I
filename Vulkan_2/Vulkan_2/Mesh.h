#pragma once

#include "Vertex.h"

namespace Engine
{
	class Buffer;
}

namespace Resource
{
	class Mesh
	{
	public:
		Mesh();
		Mesh(const char* file);
		Mesh(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices);
		~Mesh();

#pragma region Getters

		const std::vector<Vertex>& GetVertices() const { return _vertices; }
		const size_t& GetVerticesSize() const { return _vertices.size(); }
		const std::vector<uint32_t>& GetIndices() const { return _indices; }
		const size_t& GetIndicesSize() const { return _indices.size(); }

		Engine::Buffer* GetDataBuffer() { return _dataBuffer; }

#pragma endregion

	private:
		void InitializeBuffer();

	private:
		std::vector<Vertex> _vertices;
		std::vector<uint32_t> _indices;

		Engine::Buffer* _dataBuffer;
	};
}