#include "pch.h"
#include "Queue.h"

#include "Application.h"

Engine::Queue::Queue()
{
}

Engine::Queue::~Queue()
{
}

void Engine::Queue::InitializeQueue(uint32_t queueIndex)
{
	vkGetDeviceQueue(Application::s_logicalDevice, _queueFamilyIndex, queueIndex, &_queue);
}
