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