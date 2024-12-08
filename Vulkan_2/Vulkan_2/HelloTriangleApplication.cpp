#include "pch.h"
#include "HelloTriangleApplication.h"
#include <set>
#include <algorithm>
#include <limits>
#include <vector>

#include "Vertex.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Queue.h"

VkDevice HelloTriangleApplication::s_logicalDevice = VK_NULL_HANDLE;
VkPhysicalDevice HelloTriangleApplication::s_physicalDevice = VK_NULL_HANDLE;

HelloTriangleApplication::HelloTriangleApplication()
{
	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
	};
	std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

	_mesh = new Mesh(std::move(vertices), std::move(indices));
	_dataBuffer = new Engine::Buffer();
	_graphicsQueue = new Engine::Queue();
	_presentQueue = new Engine::Queue();
	_transferQueue = new Engine::Queue();
}

void HelloTriangleApplication::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	Cleanup();
}

void HelloTriangleApplication::InitWindow()
{
	// init GLFW lib
	glfwInit();

	// set window hints
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// init window
	_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Test Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, FramebufferResizeCallback);
}

void HelloTriangleApplication::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffers();
	CreateCommandPools();
	CreateDataBuffer();
	CreateCommandBuffer();
	CreateSyncObjects();
}

void HelloTriangleApplication::MainLoop()
{
	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		DrawFrame();
	}
	vkDeviceWaitIdle(s_logicalDevice);
}

void HelloTriangleApplication::Cleanup()
{
	CleanupSwapChain();

	vkDestroyBuffer(s_logicalDevice, _vertexBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, _vertexBufferMemory, nullptr);
	vkDestroyBuffer(s_logicalDevice, _indexBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, _indexBufferMemory, nullptr);
	delete _dataBuffer;

	vkDestroyPipeline(s_logicalDevice, _graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(s_logicalDevice, _pipelineLayout, nullptr);

	vkDestroyRenderPass(s_logicalDevice, _renderPass, nullptr);

	vkDestroyCommandPool(s_logicalDevice, _graphicsCommandPool, nullptr);
	vkDestroyCommandPool(s_logicalDevice, _transferCommandPool, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(s_logicalDevice, _imageReadySemaphores[i], nullptr);
		vkDestroySemaphore(s_logicalDevice, _renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(s_logicalDevice, _inFlightFences[i], nullptr);
	}

	vkDestroyDevice(s_logicalDevice, nullptr);
	vkDestroySurfaceKHR(_instance, _surface, nullptr);

	if (enableValidationLayers)
		DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

	vkDestroyInstance(_instance, nullptr);
	glfwDestroyWindow(_window);

	glfwTerminate();
}

void HelloTriangleApplication::CreateInstance()
{
	if (enableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available!!!");
	}

	// creating structs required for instantiating Vulkan Instance

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Application Name";
	appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	std::vector<const char*> glfwExtensions;

	glfwExtensions = GetRequiredExtensions(glfwExtensionCount);/*glfwGetRequiredInstanceExtensions(&glfwExtensionCount);*/

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	bool isSupportedExtensionsIncluded = CheckRequiredSupportedExtensionsPresent(glfwExtensions.data(), glfwExtensionCount);

	// create a Vulkan instance
	if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create Vulkan instance!!!");
	}
}

void HelloTriangleApplication::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> logicalDeviceQueueCreateInfos;
	// no duplicates
	std::set<uint32_t> uniqueQueueFamilies = { _graphicsQueue->GetQueueFamilyIndex(), _presentQueue->GetQueueFamilyIndex(), _transferQueue->GetQueueFamilyIndex()};

	// Create Infos for every queue family: graphics, and present
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo logicalDeviceQueueCreateInfo{};
		logicalDeviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		logicalDeviceQueueCreateInfo.queueFamilyIndex = queueFamily;
		logicalDeviceQueueCreateInfo.queueCount = 1;
		logicalDeviceQueueCreateInfo.pQueuePriorities = &queuePriority;
		logicalDeviceQueueCreateInfos.push_back(logicalDeviceQueueCreateInfo);
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures{};
	// To get features from physical device
	//vkGetPhysicalDeviceFeatures(s_physicalDevice, &physicalDeviceFeatures);

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = logicalDeviceQueueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(logicalDeviceQueueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

#pragma region DEPRICATED AND IGNORED
	// DEPRICATED AND IGNORED BY NEW VULKAN VERSIONS
	if (enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
		deviceCreateInfo.enabledLayerCount = 0;
#pragma endregion

	if (vkCreateDevice(s_physicalDevice, &deviceCreateInfo, nullptr, &s_logicalDevice) != VK_SUCCESS)
		throw std::runtime_error("Failed to create the logical device!!!");

	_graphicsQueue->InitializeQueue(0);
	_presentQueue->InitializeQueue(0);
	_transferQueue->InitializeQueue(0);

}

void HelloTriangleApplication::CreateSurface()
{
	if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a window surface");
}

bool HelloTriangleApplication::CheckRequiredSupportedExtensionsPresent(const char** extensions, uint32_t extensionCount)
{
	uint32_t extensionPropCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropCount, nullptr);
	std::vector<VkExtensionProperties> extensionProps(extensionPropCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropCount, extensionProps.data());

	for (int i = 0; i < extensionCount; i++)
	{
		bool found = false;
		for (VkExtensionProperties& prop : extensionProps)
		{
			int result = strcmp(prop.extensionName, extensions[i]);
			if (result == 0)
			{
				found = true;
				break;
			}
		}
		if (found)
			continue;
		else
			return false;
	}
	return true;
}

bool HelloTriangleApplication::CheckValidationLayerSupport()
{
	uint32_t layersCount = 0;
	vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
	std::vector<VkLayerProperties> availableLayerProps(layersCount);
	vkEnumerateInstanceLayerProperties(&layersCount, availableLayerProps.data());

	for (const char* layer : validationLayers)
	{
		bool found = false;
		for (VkLayerProperties& layerProp : availableLayerProps)
		{
			int result = strcmp(layer, layerProp.layerName);
			if (result == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}
	return true;
}

std::vector<const char*> HelloTriangleApplication::GetRequiredExtensions(uint32_t& extensionsCount)
{
	//uint32_t extensionsCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
	
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + extensionsCount);
	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	extensionsCount = extensions.size();
	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL HelloTriangleApplication::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
	VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void HelloTriangleApplication::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

void HelloTriangleApplication::SetupDebugMessenger()
{
	if (!enableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!!!");
	}
}

void HelloTriangleApplication::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("Cannot find GPUs with Vulkan support!!!");
	std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, physicalDevices.data());

	for (const VkPhysicalDevice& device : physicalDevices)
	{
		if (IsDeviceSuitable(device))
		{
			s_physicalDevice = device;
			break;
		}
	}
	if (s_physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("Cannot find suitable GPU!!!");

	QueueFamilyIndices indices = FindQueueFamily(s_physicalDevice);
	QueueFamilyIndices transferIndices = FindQueueFamily(s_physicalDevice, true);
	_graphicsQueue->SetQueueFamilyIndex(indices.graphicsFamily.value());
	_presentQueue->SetQueueFamilyIndex(indices.presentFamily.value());
	_transferQueue->SetQueueFamilyIndex(transferIndices.transferFamily.value());
}

void HelloTriangleApplication::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(s_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(swapChainSupport.surfaceFormats);
	VkPresentModeKHR presentMode = ChoosePresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapChainExtent(swapChainSupport.surfaceCapabilities);

	uint32_t imageCount = swapChainSupport.surfaceCapabilities.minImageCount + 1;
	if (swapChainSupport.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.surfaceCapabilities.maxImageCount)
		imageCount = swapChainSupport.surfaceCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = _surface;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageArrayLayers = 1; //  For non-stereoscopic-3D applications, this value is 1.
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // Use VK_IMAGE_USAGE_TRANSFER_DST_BIT to render in back buffer and present using swap chain

	uint32_t queueFamilyIndices[] = { _graphicsQueue->GetQueueFamilyIndex(), _presentQueue->GetQueueFamilyIndex() };
	if (_graphicsQueue->GetQueueFamilyIndex() != _presentQueue->GetQueueFamilyIndex())
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0; // optional to specify (only required to set if the image sharing mode is CONCURRENT)
		swapChainCreateInfo.pQueueFamilyIndices = nullptr; // optional to specify (only required to set if the image sharing mode is CONCURRENT)
	}

	swapChainCreateInfo.preTransform = swapChainSupport.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(s_logicalDevice, &swapChainCreateInfo, nullptr, &_swapChain) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Swap Chain!!!");

	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(s_logicalDevice, _swapChain, &swapChainImageCount, nullptr);
	_swapChainImages.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(s_logicalDevice, _swapChain, &swapChainImageCount, _swapChainImages.data());
	_swapChainImageFormat = surfaceFormat.format;
	_swapChainExtent = extent;
}

void HelloTriangleApplication::CreateImageViews()
{
	_swapChainImageViews.resize(_swapChainImages.size());
	for (size_t i = 0; i < _swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = _swapChainImages[i];
		imageViewCreateInfo.format = _swapChainImageFormat;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(s_logicalDevice, &imageViewCreateInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view!!!");
	}
}

void HelloTriangleApplication::CreateGraphicsPipeline()
{
	std::vector<char> vertShaderCode = ReadFile("shaders/vert.spv");
	std::vector<char> fragShaderCode = ReadFile("shaders/frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo{};
	vertShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCreateInfo.module = vertShaderModule;
	vertShaderStageCreateInfo.pName = "main";
	vertShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo{};
	fragShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCreateInfo.module = fragShaderModule;
	fragShaderStageCreateInfo.pName = "main";
	fragShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

	//
	// START CREATING ALL THE STAGES OF THE GRAPHICS PIPELINE
	//

#pragma region VERTEX INPUT
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};

	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescription = Vertex::GetAttributeDescriptions();

	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// Since we are hardcoding vertex inputs in the shader itself, we do not need to bind vertex input data. This structure will be used when creating VERTEX BUFFERS
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();
#pragma endregion

#pragma region INPUT ASSEMBLY
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // OR use VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
#pragma endregion
	
#pragma region VIEWPORT AND SCISSORS (WILL BE SETUP LATER) | UPDATE: SETUP IN recordCommandBuffer FUNCTION
	//VkViewport viewport{};
	//viewport.x = 0.0f;
	//viewport.y = 0.0f;
	//viewport.width = _swapChainExtent.width;
	//viewport.height = _swapChainExtent.height;
	//viewport.minDepth = 0.0f;
	//viewport.maxDepth = 1.0f;

	//// Scissor that covers the entire framebuffer
	//VkRect2D scissor{};
	//scissor.extent = _swapChainExtent;
	//scissor.offset = { 0, 0 };

#pragma endregion

#pragma region DYNAMIC STATE
	// Set viewport and scissor as dynamic state in the graphics pipeline
	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();
#pragma endregion

#pragma region VIEWPORT STATE (WITH DYNAMC STATE - FOR VIEWPORT AND SCISSORS)
	// Without Dynamic State, you must setup viewport and scissor like this
	//
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;
#pragma endregion

#pragma region RASTERIZATION STATE
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	// Optional values
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
#pragma endregion

#pragma region MULTISAMPLING STATE
	VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo{};
	multisamplingStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingStateCreateInfo.minSampleShading = 1.0f;
	multisamplingStateCreateInfo.pSampleMask = nullptr;
	multisamplingStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplingStateCreateInfo.alphaToOneEnable = VK_FALSE;
#pragma endregion

#pragma region COLOR BLEND STATE
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
	// enable RGBA color mask
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT;
	// not color blending for this project
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
#pragma endregion

#pragma region PIPELINE LAYOUT
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	// Create a default pipeline layout object
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(s_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout object!!!");
#pragma endregion

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = _pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = _renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	// NOT deriving from any pre existing pipeline
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(s_logicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("Failed to create the Graphics Pipeline!!!");

	// Destroy shader module as its no longer required after creating graphics pipeline
	vkDestroyShaderModule(s_logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(s_logicalDevice, fragShaderModule, nullptr);
}

void HelloTriangleApplication::CreateRenderPass()
{
#pragma region COLOR ATTACHMENT
	VkAttachmentDescription colorAttachmentDesc{};
	colorAttachmentDesc.format = _swapChainImageFormat;
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
#pragma endregion

	// We will be making only one subpass
#pragma region SUBPASS DESCRIPTION
	VkAttachmentReference colorAttachmentRef{};
	// Our array of attachment descriptions only consist of one, hence why the index is 0
	colorAttachmentRef.attachment = 0;
	// The attachment is a color buffer
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
#pragma endregion

	VkSubpassDependency subpassDependency{};
	// VK_SUBPASS_EXTERNAL specifies subpass index before the first user created subpass. 
	// This includes commands that occur before this submission order
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	// User created and the only subpass
	subpassDependency.dstSubpass = 0;
	// Wait for color attachment stage
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;

	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachmentDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(s_logicalDevice, &renderPassCreateInfo, nullptr, &_renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Render Pass!!!");
}

void HelloTriangleApplication::CreateFrameBuffers()
{
	_swapChainFramebuffers.resize(_swapChainImageViews.size());
	
	for (int i = 0; i < _swapChainFramebuffers.size(); i++)
	{
		VkImageView imageAttachments[] = { _swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = _renderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = imageAttachments;
		framebufferCreateInfo.width = _swapChainExtent.width;
		framebufferCreateInfo.height = _swapChainExtent.height;
		// single images
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(s_logicalDevice, &framebufferCreateInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Framebuffer!!!");
	}
}

void HelloTriangleApplication::CreateCommandPools()
{
	// graphics command pool
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// this is a graphics command pool. We will be using this for recording drawing commands
		commandPoolCreateInfo.queueFamilyIndex = _graphicsQueue->GetQueueFamilyIndex();

		if (vkCreateCommandPool(s_logicalDevice, &commandPoolCreateInfo, nullptr, &_graphicsCommandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the graphics command pool!!!");
	}

	// transfer command pool
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		// this is a graphics command pool. We will be using this for recording drawing commands
		commandPoolCreateInfo.queueFamilyIndex = _transferQueue->GetQueueFamilyIndex();

		if (vkCreateCommandPool(s_logicalDevice, &commandPoolCreateInfo, nullptr, &_transferCommandPool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create the transfer command pool!!!");
	}
}

void HelloTriangleApplication::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_mesh->GetVertices().at(0)) * _mesh->GetVertices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	// Create a staging buffer that acts as the source of the transfer command
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, VK_SHARING_MODE_CONCURRENT, 2);

	void* data;
	vkMapMemory(s_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _mesh->GetVertices().data(), (size_t)bufferSize);
	vkUnmapMemory(s_logicalDevice, stagingBufferMemory);

	// Create a transfer destination buffer
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory, VK_SHARING_MODE_EXCLUSIVE);

	CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(s_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(_mesh->GetIndices().at(0)) * _mesh->GetIndices().size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	// Create a staging buffer that acts as the source of the transfer command
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, VK_SHARING_MODE_CONCURRENT, 2);

	void* data;
	vkMapMemory(s_logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, _mesh->GetIndices().data(), (size_t)bufferSize);
	vkUnmapMemory(s_logicalDevice, stagingBufferMemory);

	// Create a transfer destination buffer
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory, VK_SHARING_MODE_EXCLUSIVE);

	CopyBuffer(stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(s_logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::CreateDataBuffer()
{
	VkDeviceSize verticesSize = sizeof(_mesh->GetVertices().at(0)) * _mesh->GetVerticesSize();
	VkDeviceSize indicesSize = sizeof(_mesh->GetIndices().at(0)) * _mesh->GetIndicesSize();
	VkDeviceSize bufferSize = indicesSize +	verticesSize;

	_dataBuffer->SetVertexOffset(0);
	_dataBuffer->SetIndexOffset(sizeof(_mesh->GetVertices().at(0)) * _mesh->GetVerticesSize());

	uint32_t indices[] = { _transferQueue->GetQueueFamilyIndex(), _graphicsQueue->GetQueueFamilyIndex() };
	Engine::Buffer* stagingBuffer = new Engine::Buffer();
	stagingBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_CONCURRENT, 2, indices);

	void* data;
	vkMapMemory(s_logicalDevice, stagingBuffer->GetBufferMemory(), 0, bufferSize, 0, &data);
	// copy vertices
	memcpy(static_cast<char*>(data) + _dataBuffer->GetVertexOffset(), _mesh->GetVertices().data(), verticesSize);
	// copy indices
	memcpy(static_cast<char*>(data) + _dataBuffer->GetIndexOffset(), _mesh->GetIndices().data(), indicesSize);
	vkUnmapMemory(s_logicalDevice, stagingBuffer->GetBufferMemory());

	// Create a transfer destination buffer
	_dataBuffer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);

	CopyBuffer(stagingBuffer->GetBuffer(), _dataBuffer->GetBuffer(), bufferSize);

	delete stagingBuffer;
}

void HelloTriangleApplication::CreateCommandBuffer()
{
	_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	// Now Create Command Buffers
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = _graphicsCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

	if (vkAllocateCommandBuffers(s_logicalDevice, &commandBufferAllocateInfo, _commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Command Buffers!!!");

}

void HelloTriangleApplication::CreateSyncObjects()
{
	_imageReadySemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(s_logicalDevice, &semaphoreCreateInfo, nullptr, &_imageReadySemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(s_logicalDevice, &semaphoreCreateInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(s_logicalDevice, &fenceCreateInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Sync Objects!!!");
	}
}

void HelloTriangleApplication::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
	app->_bFrameBufferResized = true;
}

bool HelloTriangleApplication::IsDeviceSuitable(VkPhysicalDevice device)
{

	////////////////////////////

	/*VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);*/

	///////////////////////////
	QueueFamilyIndices indices = FindQueueFamily(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainSupportAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupportDetails = QuerySwapChainSupport(device);
		swapChainSupportAdequate = !swapChainSupportDetails.surfaceFormats.empty() && !swapChainSupportDetails.presentModes.empty();
	}

	return indices.IsComplete() && extensionsSupported && swapChainSupportAdequate;
}

bool HelloTriangleApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionsCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const VkExtensionProperties& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	// if the array is empty, that means the required extensions are available
	return requiredExtensions.empty();
}

QueueFamilyIndices HelloTriangleApplication::FindQueueFamily(VkPhysicalDevice device, bool bExclusivelyCheckForTransfer)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertiesCount, queueFamilies.data());

	int i = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
	{
		if(bExclusivelyCheckForTransfer)
		{
			if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				indices.transferFamily = i;
			}
		}
		else
		{
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				indices.transferFamily = i;
			}

			VkBool32 isPresentSupported = false;
			// check to see if present is supported in this queue
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &isPresentSupported);
			if (isPresentSupported)
				indices.presentFamily = i;
			// check to see if this queue supports graphics commands
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;
			if (indices.IsComplete())
				break;
		}
		i++;
	}

	return indices;
}

QueueFamilyIndices HelloTriangleApplication::FindQueueFamily(VkPhysicalDevice device, uint32_t queueFamilyFlag)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyPropertiesCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertiesCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyPropertiesCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyPropertiesCount, queueFamilies.data());

	int i = 0;
	for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
	{
		if(queueFamily.queueFlags & queueFamilyFlag)
		{
			switch (queueFamilyFlag)
			{
			case VK_QUEUE_TRANSFER_BIT:
				indices.transferFamily = i;
				return indices;
				break;
			case VK_QUEUE_GRAPHICS_BIT:
				indices.graphicsFamily = i;
				return indices;
				break;
				default:
					break;
			}
		}

		i++;
	}
	if(!indices.transferFamily.has_value())
	{
		throw std::runtime_error("Failed to find a queue family with transfer support!!!");
	}
	return indices;
}

SwapChainSupportDetails HelloTriangleApplication::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails supportDetails;
	
	// Get surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &supportDetails.surfaceCapabilities);

	// Get surface formats
	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &surfaceFormatCount, nullptr);
	if (surfaceFormatCount != 0)
	{
		supportDetails.surfaceFormats.resize(surfaceFormatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &surfaceFormatCount, supportDetails.surfaceFormats.data());
	}

	// Get present modes
	uint32_t surfacePresentModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &surfacePresentModesCount, nullptr);
	if (surfacePresentModesCount != 0)
	{
		supportDetails.presentModes.resize(surfacePresentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &surfacePresentModesCount, supportDetails.presentModes.data());
	}

	return supportDetails;
}

VkSurfaceFormatKHR HelloTriangleApplication::ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
	for (VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
	{
		if (surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
			return surfaceFormat;
	}
	return surfaceFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::ChoosePresentMode(std::vector<VkPresentModeKHR> presentModes)
{
	for (VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) // try for the best one ... tripple buffering
			return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D HelloTriangleApplication::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != UINT_MAX) // UINT_MAX used instead of std::numeric_limits<uint32_t>::max() || https://www.geeksforgeeks.org/stdnumeric_limitsmax-and-stdnumeric_limitsmin-in-c/
		return surfaceCapabilities.currentExtent;
	else
	{
		int width, height;
		glfwGetFramebufferSize(_window, &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		actualExtent.width = std::clamp(actualExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
		return actualExtent;
	}
}

VkShaderModule HelloTriangleApplication::CreateShaderModule(const std::vector<char>& shaderCode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(s_logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Shader Module!!!");

	return shaderModule;
}

uint32_t HelloTriangleApplication::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(s_physicalDevice, &memoryProperties);
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!!!");
}

void HelloTriangleApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags,
	VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount)
{
	VkBufferCreateInfo vertexBufferCreateInfo{};
	vertexBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferCreateInfo.size = size;
	vertexBufferCreateInfo.usage = usageFlags;
	vertexBufferCreateInfo.sharingMode = sharingMode;
	vertexBufferCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	if(queueFamilyIndexCount > 1)
	{
		uint32_t indices[] = { _transferQueue->GetQueueFamilyIndex(), _graphicsQueue->GetQueueFamilyIndex() };
		vertexBufferCreateInfo.pQueueFamilyIndices = indices;
	}

	if (vkCreateBuffer(s_logicalDevice, &vertexBufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create Vertex Buffer!!!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(s_logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(s_logicalDevice, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate vertex buffer memory!!!");
	}

	// Bind buffer memory
	vkBindBufferMemory(s_logicalDevice, buffer, bufferMemory, 0);
}

void HelloTriangleApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// Create a temporary command buffer using the transfer command pool
	VkCommandBufferAllocateInfo cmdBufferAllocInfo{};
	cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocInfo.commandPool = _transferCommandPool;
	cmdBufferAllocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(s_logicalDevice, &cmdBufferAllocInfo, &commandBuffer);

	// Start recording the newly created command buffer
	VkCommandBufferBeginInfo cmdBufferBeginInfo{};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// begin recording the command buffer
	vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);

	// Copy the buffers
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	// end recording the command buffer
	vkEndCommandBuffer(commandBuffer);

	// submit the recorded command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_transferQueue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_transferQueue->GetQueue());

	// free up the command buffer
	vkFreeCommandBuffers(s_logicalDevice, _transferCommandPool, 1, &commandBuffer);
}

void HelloTriangleApplication::RecordCommandBuffer(VkCommandBuffer commmandBuffer, uint32_t swapChainImageIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// Optional Values
	//// set flag to none
	//commandBufferBeginInfo.flags = 0;
	//// only used in secondary command buffer
	//commandBufferBeginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commmandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording command buffer!!!");

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _renderPass;
	renderPassBeginInfo.framebuffer = _swapChainFramebuffers[swapChainImageIndex];
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = _swapChainExtent;
	
	VkClearValue clearValue;
	// Clear color, not depth stencil
	clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearValue;

	// Begin recording commands
	vkCmdBeginRenderPass(commmandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = _swapChainExtent.width;
	viewport.height = _swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commmandBuffer, 0, 1, &viewport);
	
	// Scissor that covers the entire framebuffer
	VkRect2D scissor{};
	scissor.extent = _swapChainExtent;
	scissor.offset = { 0, 0 };
	vkCmdSetScissor(commmandBuffer, 0, 1, &scissor);

	// TODO: Aliasing
	//// Bind vertex buffer to the command buffer
	//VkBuffer vertexBuffers[] = { _vertexBuffer };
	//VkDeviceSize offsets[] = { 0 };
	//vkCmdBindVertexBuffers(commmandBuffer, 0, 1, vertexBuffers, offsets);

	//// Bind index buffer to the command buffer
	//vkCmdBindIndexBuffer(commmandBuffer, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// Bind vertex buffer to the command buffer
	VkDeviceSize offsets[] = { _dataBuffer->GetVertexOffset() };
	vkCmdBindVertexBuffers(commmandBuffer, 0, 1, &_dataBuffer->GetBuffer(), offsets);
	// Bind index buffer to the command buffer
	vkCmdBindIndexBuffer(commmandBuffer, _dataBuffer->GetBuffer(), _dataBuffer->GetIndexOffset(), VK_INDEX_TYPE_UINT32);

	// Draw :)
	//vkCmdDraw(commmandBuffer, static_cast<uint32_t>(_mesh->GetVertices().size()), 1, 0, 0);
	vkCmdDrawIndexed(commmandBuffer, static_cast<uint32_t>(_mesh->GetIndices().size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commmandBuffer);

	if (vkEndCommandBuffer(commmandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to end recording Command Buffer!!!");
}

void HelloTriangleApplication::DrawFrame()
{
	// Wait for the previous frame to finish rendering
	vkWaitForFences(s_logicalDevice, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(s_logicalDevice, _swapChain, UINT64_MAX, _imageReadySemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
	if(result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image while drawing!!!");
	}

	// Only reset fence to unsignalled state if submitting any work
	vkResetFences(s_logicalDevice, 1, &_inFlightFences[_currentFrame]);

	// Use 0 as flag to make the command buffer hold onto the memory to be reused in the next recording
	vkResetCommandBuffer(_commandBuffers[_currentFrame], 0);

	RecordCommandBuffer(_commandBuffers[_currentFrame], imageIndex);

	VkSubmitInfo queueSubmitInfo{};
	queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore waitSemaphores[] = { _imageReadySemaphores[_currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	queueSubmitInfo.waitSemaphoreCount = 1;
	queueSubmitInfo.pWaitSemaphores = waitSemaphores;
	queueSubmitInfo.pWaitDstStageMask = waitStages;
	queueSubmitInfo.commandBufferCount = 1;
	queueSubmitInfo.pCommandBuffers = &_commandBuffers[_currentFrame];

	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
	queueSubmitInfo.signalSemaphoreCount = 1;
	queueSubmitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(_graphicsQueue->GetQueue(), 1, &queueSubmitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS)
		throw std::runtime_error("Failed to submit graphics queue!!!");

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	VkSwapchainKHR swapChains[] = { _swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // since we're only using a single swap chain

	// Queue the image for presentation
	result = vkQueuePresentKHR(_presentQueue->GetQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _bFrameBufferResized)
	{
		_bFrameBufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swap chain image!!!");
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::CleanupSwapChain()
{
	for (VkFramebuffer& framebuffer : _swapChainFramebuffers)
		vkDestroyFramebuffer(s_logicalDevice, framebuffer, nullptr);
	for (VkImageView& imageView : _swapChainImageViews)
		vkDestroyImageView(s_logicalDevice, imageView, nullptr);
	vkDestroySwapchainKHR(s_logicalDevice, _swapChain, nullptr);
}

void HelloTriangleApplication::RecreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	while(width == 0 || height == 0)
	{
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(s_logicalDevice);
	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateFrameBuffers();
}
