#pragma once

namespace Engine
{
	class Buffer;
	class Image;
}

namespace Resource
{
	class Material
	{
	public:
		Material(const char* file);
		~Material();

#pragma region Getters

		VkDescriptorSet& GetDescriptorSet() { return _descriptorSet; }
		Engine::Image* GetTextureImage() { return _textureImage; }

#pragma endregion

	private:
		VkDescriptorSet _descriptorSet;
		Engine::Image* _textureImage;
	};
}