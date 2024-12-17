#include "pch.h"
#include "Sampler.h"

#include "Application.h"

Engine::Sampler::Sampler()
{
}

Engine::Sampler::~Sampler()
{
	vkDestroySampler(Application::s_logicalDevice, _sampler, nullptr);
}

void Engine::Sampler::CreateSampler(VkSamplerCreateInfo* createInfo)
{
	if (vkCreateSampler(Application::s_logicalDevice, createInfo, nullptr, &_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!!!");
	}
}
