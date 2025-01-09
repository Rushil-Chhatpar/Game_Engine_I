#include "pch.h"
#include "Material.h"
#include "Buffer.h"
#include <stb_image.h>
#include "Queue.h"

#include "Application.h"
#include "Image.h"

Resource::Material::Material(const char* file)
{
	// load the image
	int texWidth, texHeight, texChannels;
	stbi_uc* pixelData = stbi_load("textures/IMG_Bake_Diffuse.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha;

	if (!pixelData)
	{
		throw std::runtime_error("Failed to load texture image!!!");
	}

	// bind to staging buffer
	Engine::Buffer* stagingBuffer = new Engine::Buffer();
	uint32_t indices[] = {Application::s_transferQueue->GetQueueFamilyIndex(), Application::s_graphicsQueue->GetQueueFamilyIndex() };
	stagingBuffer->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_CONCURRENT, 2, indices);

	void* data;
	vkMapMemory(Application::s_logicalDevice, stagingBuffer->GetBufferMemory(), 0, imageSize, 0, &data);
	memcpy(data, pixelData, static_cast<size_t>(imageSize));
	vkUnmapMemory(Application::s_logicalDevice, stagingBuffer->GetBufferMemory());

	// free pixel Data
	stbi_image_free(pixelData);

	_textureImage = new Engine::Image();
	Engine::EngineImageCreateInfo imageCreateInfo{};
	imageCreateInfo.size = imageSize;
	imageCreateInfo.width = texWidth;
	imageCreateInfo.height = texHeight;
	imageCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.imageTiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	imageCreateInfo.queueFamilyIndexCount = 2;
	imageCreateInfo.queueFamilyIndices = indices;

	//_textureImage->CreateImage(imageSize, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_CONCURRENT, 2, indices);
	_textureImage->CreateImage(&imageCreateInfo);

	VkCommandBuffer commandBuffer = Application::BeginSingleTimeCommands();
	_textureImage->TransitionImageLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	Application::EndSingleTimeCommands(commandBuffer);

	Application::CopyBufferToImage(stagingBuffer, _textureImage, _textureImage->GetWidth(), _textureImage->GetHeight());

	commandBuffer = Application::BeginSingleTimeCommands();
	_textureImage->TransitionImageLayout(commandBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	Application::EndSingleTimeCommands(commandBuffer);

	delete stagingBuffer;

	_textureImage->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);

	
}

Resource::Material::~Material()
{
	delete _textureImage;
}
