#pragma once

namespace Engine
{
	class Buffer
	{
	public:
		Buffer();
		~Buffer();

		VkBuffer& GetBuffer() { return _buffer; }
		VkDeviceMemory& GetBufferMemory() { return _bufferMemory; }

	private:
		VkBuffer _buffer;
		VkDeviceMemory _bufferMemory;
	};


}
