#pragma once

namespace Engine
{
	class Sampler
	{
	public:
		Sampler();
		~Sampler();

	private:

	public:
#pragma region Getters

		VkSampler& Get() { return _sampler; }

#pragma endregion

#pragma region Setters



#pragma endregion

	private:
		VkSampler _sampler;

	};
}