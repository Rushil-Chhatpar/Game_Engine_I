#include "pch.h"
#include "Application.h"
#include <set>
#include <algorithm>
#include <chrono>
#include <limits>
#include <vector>

#include "Vertex.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Image.h"
#include "Queue.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Sampler.h"

VkDevice Application::s_logicalDevice = VK_NULL_HANDLE;
VkPhysicalDevice Application::s_physicalDevice = VK_NULL_HANDLE;
VkPhysicalDeviceProperties Application::s_physicalDeviceProperties{};

Application::Application()
{
	// positive X is left, negative X is right
	// positive Y is up, negative Y is down
	// positive Z is forward, negative Z is back
	std::vector<Vertex> vertices = {
		// bottom left
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
		// bottom right
		{{0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		// top right
		{{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		// top left
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},

		// bottom left
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
		// bottom right
		{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		// top right
		{{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
		// top left
		{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	};
	std::vector<uint32_t> indices = { \
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4,
	};

	//_mesh = new Mesh(std::move(vertices), std::move(indices));
	_mesh = new Mesh("models/Sitting.obj");

	_dataBuffer = new Engine::Buffer();
	_graphicsQueue = new Engine::Queue();
	_presentQueue = new Engine::Queue();
	_transferQueue = new Engine::Queue();
}

void Application::Run()
{
	InitWindow();
	InitVulkan();
	MainLoop();
	Cleanup();
}

void Application::InitWindow()
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

void Application::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPools();
	CreateDepthResources();
	CreateFrameBuffers();
	CreateTextureImage();
	CreateTextureImageView();
	CreateTextureSampler();
	CreateDataBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffer();
	CreateSyncObjects();
}

void Application::MainLoop()
{
	while (!glfwWindowShouldClose(_window))
	{
		glfwPollEvents();
		DrawFrame();
	}
	vkDeviceWaitIdle(s_logicalDevice);
}

void Application::Cleanup()
{
	CleanupSwapChain();

	vkDestroyBuffer(s_logicalDevice, _vertexBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, _vertexBufferMemory, nullptr);
	vkDestroyBuffer(s_logicalDevice, _indexBuffer, nullptr);
	vkFreeMemory(s_logicalDevice, _indexBufferMemory, nullptr);
	delete _dataBuffer;

	for (Engine::Buffer* buffer : _uniformBuffers)
	{
		delete buffer;
	}

	delete _textureImage;
	delete _textureSampler;

	delete _depthImage;

	vkDestroyDescriptorPool(s_logicalDevice, _descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(s_logicalDevice, _descriptorSetLayout, nullptr);

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

void Application::CreateInstance()
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

void Application::CreateLogicalDevice()
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
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

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

void Application::CreateSurface()
{
	if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
		throw std::runtime_error("Failed to create a window surface");
}

bool Application::CheckRequiredSupportedExtensionsPresent(const char** extensions, uint32_t extensionCount)
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

bool Application::CheckValidationLayerSupport()
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

std::vector<const char*> Application::GetRequiredExtensions(uint32_t& extensionsCount)
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

VKAPI_ATTR VkBool32 VKAPI_CALL Application::DebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
	VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
	void* pUserData)
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
	createInfo.pUserData = nullptr;
}

VkFormat Application::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(s_physicalDevice, format, &properties);
		if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!!!");
}

bool Application::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Application::SetupDebugMessenger()
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

void Application::PickPhysicalDevice()
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

	vkGetPhysicalDeviceProperties(s_physicalDevice, &s_physicalDeviceProperties);
}

void Application::CreateSwapChain()
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

void Application::CreateImageViews()
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

void Application::CreateDescriptorSetLayout()
{
#pragma region Uniform Buffer Object (UBO)

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// UBO is bound to 0
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	// no samplers
	uboLayoutBinding.pImmutableSamplers = nullptr;

#pragma endregion

#pragma region Sampler

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// Sampler is bound to 1
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

#pragma endregion

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutCreateInfo.pBindings = bindings.data();
	
	if(vkCreateDescriptorSetLayout(s_logicalDevice, &layoutCreateInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout!!!");
	}
}

void Application::CreateGraphicsPipeline()
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
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &_descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	if (vkCreatePipelineLayout(s_logicalDevice, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout object!!!");
#pragma endregion

#pragma region DEPTH STENCIL STATE

	VkPipelineDepthStencilStateCreateInfo depthStencilState{};
	depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;
	// disable stencil for now
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = {};
	depthStencilState.back = {};

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
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
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

void Application::CreateRenderPass()
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

	VkAttachmentReference colorAttachmentRef{};
	// Our array of attachment descriptions only consist of one, hence why the index is 0
	colorAttachmentRef.attachment = 0;
	// The attachment is a color buffer
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#pragma endregion

#pragma region DEPTH ATTACHMENT

	VkAttachmentDescription depthAttachmentDesc{};
	depthAttachmentDesc.format = FindSupportedDepthFormat();
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

	if (vkCreateRenderPass(s_logicalDevice, &renderPassCreateInfo, nullptr, &_renderPass) != VK_SUCCESS)
		throw std::runtime_error("Failed to create Render Pass!!!");
}

void Application::CreateFrameBuffers()
{
	_swapChainFramebuffers.resize(_swapChainImageViews.size());

	VkImageView depthImageView = _depthImage->GetImageView();

	for (int i = 0; i < _swapChainFramebuffers.size(); i++)
	{
		std::array<VkImageView,2> imageAttachments = { _swapChainImageViews[i], depthImageView };

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = _renderPass;
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageAttachments.size());
		framebufferCreateInfo.pAttachments = imageAttachments.data();
		framebufferCreateInfo.width = _swapChainExtent.width;
		framebufferCreateInfo.height = _swapChainExtent.height;
		// single images
		framebufferCreateInfo.layers = 1;

		if (vkCreateFramebuffer(s_logicalDevice, &framebufferCreateInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("Failed to create Framebuffer!!!");
	}
}

void Application::CreateCommandPools()
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

void Application::CreateDepthResources()
{
	VkFormat depthFormat = FindSupportedDepthFormat();

	if (_depthImage != nullptr)
		delete _depthImage;

	_depthImage = new Engine::Image();
	Engine::EngineImageCreateInfo imageCreateInfo{};
	imageCreateInfo.width = _swapChainExtent.width;
	imageCreateInfo.height = _swapChainExtent.height;
	imageCreateInfo.imageFormat = depthFormat;
	imageCreateInfo.imageTiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	_depthImage->CreateImage(&imageCreateInfo);
	_depthImage->CreateImageView(VK_IMAGE_ASPECT_DEPTH_BIT);

	TransitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void Application:: CreateTextureImage()
{
	// load the image
	int texWidth, texHeight, texChannels;
	stbi_uc* pixelData = stbi_load("textures/IMG_Bake_Diffuse.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = texWidth * texHeight * STBI_rgb_alpha;

	if(!pixelData)
	{
		throw std::runtime_error("Failed to load texture image!!!");
	}

	// bind to staging buffer
	Engine::Buffer* stagingBuffer = new Engine::Buffer();
	uint32_t indices[] = { _transferQueue->GetQueueFamilyIndex(), _graphicsQueue->GetQueueFamilyIndex() };
	stagingBuffer->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_CONCURRENT, 2, indices);

	void* data;
	vkMapMemory(s_logicalDevice, stagingBuffer->GetBufferMemory(), 0, imageSize, 0, &data);
	memcpy(data, pixelData, static_cast<size_t>(imageSize));
	vkUnmapMemory(s_logicalDevice, stagingBuffer->GetBufferMemory());

	// free pixel Data
	stbi_image_free(pixelData);

	_textureImage = new Engine::Image();
	Engine::EngineImageCreateInfo imageCreateInfo{};
	imageCreateInfo.size = imageSize;
	imageCreateInfo.width = texWidth;
	imageCreateInfo.height = texHeight;
	imageCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	imageCreateInfo.imageTiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	imageCreateInfo.queueFamilyIndexCount = 2;
	imageCreateInfo.queueFamilyIndices = indices;

	//_textureImage->CreateImage(imageSize, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
	//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_CONCURRENT, 2, indices);
	_textureImage->CreateImage(&imageCreateInfo);

	TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(stagingBuffer, _textureImage, _textureImage->GetWidth(), _textureImage->GetHeight());
	TransitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	delete stagingBuffer;
}

void Application::CreateTextureImageView()
{
	_textureImage->CreateImageView(VK_IMAGE_ASPECT_COLOR_BIT);
}

void Application::CreateTextureSampler()
{
	_textureSampler = new Engine::Sampler();

	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = s_physicalDeviceProperties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;
	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (vkCreateSampler(Application::s_logicalDevice, &createInfo, nullptr, &_textureSampler->Get()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler!!!");
	}
}

void Application::CreateVertexBuffer()
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

void Application::CreateIndexBuffer()
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

void Application::CreateUniformBuffers()
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

	for(int i=0;i<MAX_FRAMES_IN_FLIGHT;i++)
	{
		_uniformBuffers[i] = new Engine::Buffer();
		_uniformBuffers[i]->CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_EXCLUSIVE);
		// persistent mapping, no need to unmap, only unmap before the end of the application
		vkMapMemory(s_logicalDevice, _uniformBuffers[i]->GetBufferMemory(), 0, bufferSize, 0, &_uniformBuffersMapped[i]);
	}
}

void Application::CreateDescriptorPool()
{
	std::array<VkDescriptorPoolSize, 2>  poolSize{};
	poolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
	createInfo.pPoolSizes = poolSize.data();
	createInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if(vkCreateDescriptorPool(s_logicalDevice, &createInfo, nullptr, &_descriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool!!!");
	}
}

void Application::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = _descriptorPool;
	allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocateInfo.pSetLayouts = layouts.data();

	_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if(vkAllocateDescriptorSets(s_logicalDevice, &allocateInfo, _descriptorSets.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate descriptor sets!!!");
	}

	// Update descriptor sets
	UpdateDescriptorSets();
}

void Application::CreateDataBuffer()
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

void Application::CreateCommandBuffer()
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

void Application::CreateSyncObjects()
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

void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->_bFrameBufferResized = true;
}

bool Application::IsDeviceSuitable(VkPhysicalDevice device)
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

	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &physicalDeviceFeatures);

	return indices.IsComplete() && extensionsSupported && swapChainSupportAdequate && physicalDeviceFeatures.samplerAnisotropy;
}

bool Application::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

QueueFamilyIndices Application::FindQueueFamily(VkPhysicalDevice device, bool bExclusivelyCheckForTransfer)
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

QueueFamilyIndices Application::FindQueueFamily(VkPhysicalDevice device, uint32_t queueFamilyFlag)
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

SwapChainSupportDetails Application::QuerySwapChainSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR Application::ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> surfaceFormats)
{
	for (VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
	{
		if (surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB)
			return surfaceFormat;
	}
	return surfaceFormats[0];
}

VkPresentModeKHR Application::ChoosePresentMode(std::vector<VkPresentModeKHR> presentModes)
{
	for (VkPresentModeKHR& presentMode : presentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) // try for the best one ... tripple buffering
			return presentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
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

VkShaderModule Application::CreateShaderModule(const std::vector<char>& shaderCode)
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

uint32_t Application::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

void Application::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags,
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

void Application::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	// Create a temporary command buffer using the transfer command pool
	VkCommandBuffer commandBuffer = BeginSingleTimeTransferCommands();

	// Copy the buffers
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	// end recording the command buffer
	EndSingleTimeTransferCommands(commandBuffer);
}

void Application::CopyBufferToImage(Engine::Buffer* buffer, Engine::Image* image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0,0,0 };
	region.imageExtent = { width, height , 1};

	vkCmdCopyBufferToImage(commandBuffer, buffer->GetBuffer(), image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(commandBuffer);
}

void Application::UpdateDescriptorSets()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _uniformBuffers[i]->GetBuffer();
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = _textureImage->GetImageView();
		imageInfo.sampler = _textureSampler->Get();

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = _descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].pBufferInfo = nullptr;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(s_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

VkCommandBuffer Application::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	// safe choice to get command pool with both graphics and transfer queues
	allocInfo.commandPool = _graphicsCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(s_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Application::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	
	vkQueueSubmit(_graphicsQueue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueue->GetQueue());

	vkFreeCommandBuffers(s_logicalDevice, _graphicsCommandPool, 1, &commandBuffer);
}

VkCommandBuffer Application::BeginSingleTimeTransferCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	// safe choice to get command pool with both graphics and transfer queues
	allocInfo.commandPool = _transferCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(s_logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Application::EndSingleTimeTransferCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_transferQueue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_transferQueue->GetQueue());

	vkFreeCommandBuffers(s_logicalDevice, _transferCommandPool, 1, &commandBuffer);
}

void Application::TransitionImageLayout(Engine::Image* image, VkFormat format, VkImageLayout oldLayout,
	VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	image->TransitionImageLayout(commandBuffer, format, oldLayout, newLayout);

	EndSingleTimeCommands(commandBuffer);
}

VkFormat Application::FindSupportedDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, 
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void Application::RecordCommandBuffer(VkCommandBuffer commmandBuffer, uint32_t swapChainImageIndex)
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commmandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
		throw std::runtime_error("Failed to start recording command buffer!!!");

	VkRenderPassBeginInfo renderPassBeginInfo{};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _renderPass;
	renderPassBeginInfo.framebuffer = _swapChainFramebuffers[swapChainImageIndex];
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = _swapChainExtent;
	
	std::array<VkClearValue, 2> clearValues;
	// Clear color, not depth stencil
	clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	// depth is 1 for furthest possible initial depth value
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();

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

	// Bind vertex buffer to the command buffer
	VkDeviceSize offsets[] = { _dataBuffer->GetVertexOffset() };
	vkCmdBindVertexBuffers(commmandBuffer, 0, 1, &_dataBuffer->GetBuffer(), offsets);
	// Bind index buffer to the command buffer
	vkCmdBindIndexBuffer(commmandBuffer, _dataBuffer->GetBuffer(), _dataBuffer->GetIndexOffset(), VK_INDEX_TYPE_UINT32);

	// Bind UBOs
	vkCmdBindDescriptorSets(commmandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[_currentFrame], 0, nullptr);

	// Draw :)
	vkCmdDrawIndexed(commmandBuffer, static_cast<uint32_t>(_mesh->GetIndices().size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(commmandBuffer);

	if (vkEndCommandBuffer(commmandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to end recording Command Buffer!!!");
}

void Application::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	// time in seconds since rendering has started with floating point 
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f),  time * glm::radians(90.0f), glm::vec3(0, 1.0f, 0.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 0.0f, 15.0f), glm::vec3(0, 0, 0), glm::vec3(0, 1.0f, 0.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 20.0f);
	// flip the scaling factor
	ubo.proj[1][1] *= -1;
	memcpy(_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void Application::DrawFrame()
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

	UpdateUniformBuffer(_currentFrame);

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

void Application::CleanupSwapChain()
{
	for (VkFramebuffer& framebuffer : _swapChainFramebuffers)
		vkDestroyFramebuffer(s_logicalDevice, framebuffer, nullptr);
	for (VkImageView& imageView : _swapChainImageViews)
		vkDestroyImageView(s_logicalDevice, imageView, nullptr);
	vkDestroySwapchainKHR(s_logicalDevice, _swapChain, nullptr);
}

void Application::RecreateSwapChain()
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
	CreateDepthResources();
	CreateFrameBuffers();
}
