#include "pch.h"
#include "RenderPass.h"

#include "Application.h"

Engine::RenderPass::RenderPass()
{
}

Engine::RenderPass::~RenderPass()
{
	vkDestroyRenderPass(Application::s_logicalDevice, _renderPass, nullptr);
}

void Engine::RenderPass::CreateRenderPass(VkFormat swapChainImageFormat, VkFormat supportedDepthFormat)
{
#pragma region COLOR ATTACHMENT
	VkAttachmentDescription colorAttachmentDesc{};
	colorAttachmentDesc.format = swapChainImageFormat;
	// No multisampling yet
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;

	// Clear the framebuffer to black(null) before drawing
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// Store the memory to then render it later on the screen
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	// We are not using any special effects currently, so we do not care about the stencil buffer
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// We do not care what layout the image is in before the render pass, since we will be clearing every color data before a render pass
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// We will be converting our image to be used by swap chain to present to the screen
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	// Our array of attachment descriptions only consist of one, hence why the index is 0
	colorAttachmentRef.attachment = 0;
	// The attachment is a color buffer
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#pragma endregion

#pragma region DEPTH ATTACHMENT

	VkAttachmentDescription depthAttachmentDesc{};
	depthAttachmentDesc.format = supportedDepthFormat;
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

#pragma endregion

	// We will be making only one subpass
#pragma region SUBPASS DESCRIPTION

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
#pragma endregion

	VkSubpassDependency subpassDependency{};
	// VK_SUBPASS_EXTERNAL specifies subpass index before the first user created subpass. 
	// This includes commands that occur before this submission order
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// User created and the only subpass
	subpassDependency.dstSubpass = 0;
	// Wait for color attachment stage
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = 0;

	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> pAttachments = { colorAttachmentDesc, depthAttachmentDesc };

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(pAttachments.size());
	renderPassCreateInfo.pAttachments = pAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(Application::s_logicalDevice, &renderPassCreateInfo, nullptr, &_renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Render Pass!!!");
}
