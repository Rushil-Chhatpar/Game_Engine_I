#pragma once

namespace Engine
{
	class Image
	{
	public:
		Image();
		~Image();

		void CreateImage(VkDeviceSize size, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, 
			VkMemoryPropertyFlags properties, VkSharingMode sharingMode);
		void CreateImage(VkDeviceSize size, uint32_t width, uint32_t height, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usageFlags, 
			VkMemoryPropertyFlags properties, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount, uint32_t* queueFamilyIndices);

		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	public:
#pragma region Getters

		VkImage& GetImage() { return _image; }
		VkDeviceMemory& GetImageMemory() { return _imageMemory; }

		uint32_t GetWidth() { return _width; }
		uint32_t GetHeight() { return _height; }
		VkDeviceSize GetImageSize() { return _imageSize; }

#pragma endregion

#pragma region Setters



#pragma endregion

	private:
		VkImage _image;
		VkDeviceMemory _imageMemory;

		uint32_t _width;
		uint32_t _height;
		VkDeviceSize _imageSize;
	};
}