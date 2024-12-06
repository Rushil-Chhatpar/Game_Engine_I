#include "pch.h"
#include "Queue.h"

#include "HelloTriangleApplication.h"

Engine::Queue::Queue()
{
}

Engine::Queue::~Queue()
{
}

void Engine::Queue::InitializeQueue(uint32_t queueIndex)
{
	vkGetDeviceQueue(HelloTriangleApplication::s_logicalDevice, _queueFamilyIndex, queueIndex, &_queue);
}
