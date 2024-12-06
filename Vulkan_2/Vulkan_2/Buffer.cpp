#include "pch.h"
#include "Buffer.h"

#include "HelloTriangleApplication.h"

Engine::Buffer::Buffer()
{
}

Engine::Buffer::~Buffer()
{
	vkDestroyBuffer(HelloTriangleApplication::s_logicalDevice, _buffer, nullptr);
	vkFreeMemory(HelloTriangleApplication::s_logicalDevice, _bufferMemory, nullptr);
}
