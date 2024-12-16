#include "pch.h"
#include "Buffer.h"

#include "Application.h"

Engine::Buffer::Buffer()
{
}

Engine::Buffer::~Buffer()
{
	vkDestroyBuffer(Application::s_logicalDevice, _buffer, nullptr);
	vkFreeMemory(Application::s_logicalDevice, _bufferMemory, nullptr);
}

void Engine::Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties,
	VkSharingMode sharingMode)
{
	VkBufferCreateInfo vertexBufferCreateInfo{};
	vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferCreateInfo.size = size;
	vertexBufferCreateInfo.usage = usageFlags;
	vertexBufferCreateInfo.sharingMode = sharingMode;

	if (vkCreateBuffer(Application::s_logicalDevice, &vertexBufferCreateInfo, nullptr, &_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vertex Buffer!!!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(Application::s_logicalDevice, _buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Application::s_logicalDevice, &allocateInfo, nullptr, &_bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!!!");
	}

	// Bind buffer memory
	vkBindBufferMemory(Application::s_logicalDevice, _buffer, _bufferMemory, 0);
}

void Engine::Buffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, 
	VkSharingMode sharingMode, uint32_t queueFamilyIndexCount,
	uint32_t* queueFamilyIndices)
{
	VkBufferCreateInfo vertexBufferCreateInfo{};
	vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferCreateInfo.size = size;
	vertexBufferCreateInfo.usage = usageFlags;
	vertexBufferCreateInfo.sharingMode = sharingMode;
	vertexBufferCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	vertexBufferCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	

	if (vkCreateBuffer(Application::s_logicalDevice, &vertexBufferCreateInfo, nullptr, &_buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vertex Buffer!!!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(Application::s_logicalDevice, _buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Application::s_logicalDevice, &allocateInfo, nullptr, &_bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!!!");
	}

	// Bind buffer memory
	vkBindBufferMemory(Application::s_logicalDevice, _buffer, _bufferMemory, 0);
}

uint32_t Engine::Buffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(Application::s_physicalDevice, &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!!!");
}
