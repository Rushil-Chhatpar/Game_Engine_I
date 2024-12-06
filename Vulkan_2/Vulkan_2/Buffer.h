#pragma once

namespace Engine
{
	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, VkSharingMode sharingMode);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, uint32_t* queueFamilyIndices);

	private:
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	public:
#pragma region Getters

		VkBuffer& GetBuffer() { return _buffer; }
		VkDeviceMemory& GetBufferMemory() { return _bufferMemory; }

		VkDeviceSize& GetVertexOffset() { return _vertexOffset; }
		VkDeviceSize& GetIndexOffset() { return _indexOffset; }

#pragma endregion

#pragma region Setters

		void SetVertexOffset(VkDeviceSize offset) { _vertexOffset = offset; }
		void SetIndexOffset(VkDeviceSize offset) { _indexOffset = offset; }

#pragma endregion

	private:
		VkBuffer _buffer;
		VkDeviceMemory _bufferMemory;
		VkDeviceSize _vertexOffset;
		VkDeviceSize _indexOffset;
	};
}
