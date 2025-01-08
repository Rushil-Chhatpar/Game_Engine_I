#pragma once

namespace Engine
{
	class RenderPass
	{
	public:
		RenderPass();
		~RenderPass();
		void CreateRenderPass(VkFormat swapChainImageFormat, VkFormat supportedDepthFormat);

#pragma region Getters

		VkRenderPass& GetRenderPass() { return _renderPass; }

#pragma endregion

	private:
		VkRenderPass _renderPass;
	};

}