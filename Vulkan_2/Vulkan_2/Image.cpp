#include "pch.h"
#include "Image.h"

#include "Application.h"

Engine::Image::Image()
{

}

Engine::Image::~Image()
{
	vkDestroyImageView(Application::s_logicalDevice, _imageView, nullptr);
	vkDestroyImage(Application::s_logicalDevice, _image, nullptr);
	vkFreeMemory(Application::s_logicalDevice, _imageMemory, nullptr);
}

void Engine::Image::CreateImage(const EngineImageCreateInfo* createInfo)
{
	_width = createInfo->width;
	_height = createInfo->height;
	_imageSize = createInfo->size;
	_imageFormat = createInfo->imageFormat;
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = createInfo->width;
	imageCreateInfo.extent.height = createInfo->height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = createInfo->imageFormat;
	imageCreateInfo.tiling = createInfo->imageTiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = createInfo->usageFlags;
	imageCreateInfo.sharingMode = createInfo->sharingMode;
	imageCreateInfo.queueFamilyIndexCount = createInfo->queueFamilyIndexCount;
	imageCreateInfo.pQueueFamilyIndices = createInfo->queueFamilyIndices;
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
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, createInfo->properties);

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

	if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if(Application::HasStencilComponent(format))
		{
			memoryBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;		
	}

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
	else if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		memoryBarrier.srcAccessMask = 0;
		memoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!!!");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 
		0, 0, 
		0, 0, 
		1, &memoryBarrier);
}

void Engine::Image::CreateImageView(VkImageAspectFlags aspecFlags)
{
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = _image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = _imageFormat;
	createInfo.subresourceRange.aspectMask = aspecFlags;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;

	if(vkCreateImageView(Application::s_logicalDevice, &createInfo, nullptr, &_imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("Faioled to create texture image view!!!");
	}
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
