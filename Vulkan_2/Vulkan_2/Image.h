#pragma once

namespace Engine
{

	struct EngineImageCreateInfo
	{
		VkDeviceSize size = 0;
		uint32_t width;
		uint32_t height;
		VkFormat imageFormat;
		VkImageTiling imageTiling;
		VkImageUsageFlags usageFlags;
		VkMemoryPropertyFlags properties;
		VkSharingMode sharingMode;
		uint32_t queueFamilyIndexCount = 0;
		uint32_t* queueFamilyIndices = nullptr;
	};

	class Image
	{
	public:
		Image();
		~Image();

		void CreateImage(const EngineImageCreateInfo* createInfo);

		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

		void CreateImageView(VkImageAspectFlags aspecFlags);

	private:
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	public:
#pragma region Getters

		VkImage& GetImage() { return _image; }
		VkDeviceMemory& GetImageMemory() { return _imageMemory; }
		VkImageView& GetImageView() { return _imageView; }

		uint32_t GetWidth() { return _width; }
		uint32_t GetHeight() { return _height; }
		VkDeviceSize GetImageSize() { return _imageSize; }
		VkFormat GetImageFormat() { return _imageFormat; }

#pragma endregion

#pragma region Setters



#pragma endregion

	private:
		VkImage _image;
		VkDeviceMemory _imageMemory;
		VkImageView _imageView;

		uint32_t _width;
		uint32_t _height;
		VkDeviceSize _imageSize;
		VkFormat _imageFormat;
	};
}