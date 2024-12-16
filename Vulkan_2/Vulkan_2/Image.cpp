#include "pch.h"
#include "Image.h"

#include "Application.h"

Engine::Image::Image()
{

}

Engine::Image::~Image()
{
	vkDestroyImage(Application::s_logicalDevice, _image, nullptr);
	vkFreeMemory(Application::s_logicalDevice, _imageMemory, nullptr);
}

void Engine::Image::CreateImage(VkDeviceSize size, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags properties,
	VkSharingMode sharingMode)
{
	_width = width;
	_height = height;
	_imageSize = size;
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = imageFormat;
	imageCreateInfo.tiling = imageTiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.sharingMode = sharingMode;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	if (vkCreateImage(Application::s_logicalDevice, &imageCreateInfo, nullptr, &_image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image!!!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(Application::s_logicalDevice, _image, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Application::s_logicalDevice, &allocateInfo, nullptr, &_imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory!!!");
	}

	vkBindImageMemory(Application::s_logicalDevice, _image, _imageMemory, 0);
}

void Engine::Image::CreateImage(VkDeviceSize size, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags properties,
	VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, uint32_t* queueFamilyIndices)
{
	_width = width;
	_height = height;
	_imageSize = size;
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = imageFormat;
	imageCreateInfo.tiling = imageTiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usageFlags;
	imageCreateInfo.sharingMode = sharingMode;
	imageCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	if (vkCreateImage(Application::s_logicalDevice, &imageCreateInfo, nullptr, &_image) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image!!!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(Application::s_logicalDevice, _image, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Application::s_logicalDevice, &allocateInfo, nullptr, &_imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory!!!");
	}

	vkBindImageMemory(Application::s_logicalDevice, _image, _imageMemory, 0);
}

void Engine::Image::TransitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier memoryBarrier{};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memoryBarrier.oldLayout = oldLayout;
	memoryBarrier.newLayout = newLayout;
	memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memoryBarrier.image = _image;

	memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	memoryBarrier.subresourceRange.baseArrayLayer = 0;
	memoryBarrier.subresourceRange.layerCount = 1;
	memoryBarrier.subresourceRange.baseMipLevel = 0;
	memoryBarrier.subresourceRange.levelCount = 1;

	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	}

	memoryBarrier.srcAccessMask = 0; // TODO
	memoryBarrier.dstAccessMask = 0; // TODO

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 
		0, 0, 
		0, 0, 
		1, &memoryBarrier);
}

uint32_t Engine::Image::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
