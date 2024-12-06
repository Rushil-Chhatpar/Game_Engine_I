#pragma once

namespace Engine
{
	class Queue
	{
	public:
		Queue();
		~Queue();

#pragma region Getters

		VkQueue& GetQueue() { return _queue; }
		uint32_t& GetQueueFamilyIndex() { return _queueFamilyIndex; }

#pragma endregion

#pragma region Setters

		void SetQueueFamilyIndex(uint32_t index) { _queueFamilyIndex = index; }

#pragma endregion

	private:
		VkQueue _queue;
		uint32_t _queueFamilyIndex;
	};
}
