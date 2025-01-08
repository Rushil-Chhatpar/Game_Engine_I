#pragma once

namespace Engine
{
	struct EngineGraphicsPipelineCreateInfo
	{
		VkPipelineShaderStageCreateInfo* shaderStages;

	};

	class GraphicsPipeline
	{
	public:
		GraphicsPipeline();
		~GraphicsPipeline();
		void CreateGraphicsPipeline(VkPipelineShaderStageCreateInfo* shaderStages, VkDescriptorSetLayout descriptorSetLayout, VkRenderPass renderPass);

#pragma region Getters

		VkPipelineLayout& GetPipelineLayout() { return _pipelineLayout; }
		VkPipeline& GetGraphicsPipeline() { return _graphicsPipeline; }

#pragma endregion

	private:
		VkPipelineLayout _pipelineLayout;
		VkPipeline _graphicsPipeline;
	};
}