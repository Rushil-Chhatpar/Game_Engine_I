#pragma once
#include "Constants.h"

class Mesh;

static std::vector<char> ReadFile(const std::string& fileName)
{
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Failed to open the file!!!");

	size_t size = (size_t)file.tellg();
	std::vector<char> buffer(size);
	file.seekg(0);
	file.read(buffer.data(), size);
	file.close();

	return buffer;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, messenger, pAllocator);
}

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;

	bool IsComplete()
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> surfaceFormats;
	std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication
{
public:
	HelloTriangleApplication();
	void Run();

private: // Run functions
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void Cleanup();

private:
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void SetupDebugMessenger();
	void PickPhysicalDevice();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFrameBuffers();
	void CreateCommandPools();
	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateDataBuffer();
	void CreateCommandBuffer();
	void CreateSyncObjects();

	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private: // helper functions
	// returns QueueFamilyIndices with graphics and present queues support
	QueueFamilyIndices FindQueueFamily(VkPhysicalDevice device, bool bExclusivelyCheckForTransfer = false);
	// looks for a specific queue family flag
	QueueFamilyIndices FindQueueFamily(VkPhysicalDevice device, uint32_t queueFamilyFlag);

	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> surfaceFormats);
	VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR> presentModes);
	VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode, uint32_t queueFamilyIndexCount = 0);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:
	void RecordCommandBuffer(VkCommandBuffer commmandBuffer, uint32_t swapChainImageIndex);
	void DrawFrame();
	void CleanupSwapChain();
	void RecreateSwapChain();

private: // util functions
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	bool CheckRequiredSupportedExtensionsPresent(const char** extensions, uint32_t extensionCount);
	bool CheckValidationLayerSupport();
	std::vector<const char*> GetRequiredExtensions(uint32_t& extensionsCount);
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

private:
	GLFWwindow* _window;
	VkInstance _instance;
	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapChain;
	
	VkPipelineLayout _pipelineLayout;
	VkRenderPass _renderPass;
	VkPipeline _graphicsPipeline;

	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _logicalDevice;

	VkDebugUtilsMessengerEXT _debugMessenger;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;
	VkQueue _transferQueue;

	uint32_t _graphicsQueueFamilyIndex;
	uint32_t _presentQueueFamilyIndex;
	uint32_t _transferQueueFamilyIndex;

	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	std::vector<VkFramebuffer> _swapChainFramebuffers;

	VkCommandPool _graphicsCommandPool;
	VkCommandPool _transferCommandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	VkBuffer _vertexBuffer;
	VkDeviceMemory _vertexBufferMemory;
	VkBuffer _indexBuffer;
	VkDeviceMemory _indexBufferMemory;
	VkBuffer _dataBuffer;
	VkDeviceMemory _dataBufferMemory;

	uint32_t _currentFrame = 0;

	std::vector<VkSemaphore> _imageReadySemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;

	bool _bFrameBufferResized = false;


#pragma region Move this to Component System
	Mesh* _mesh;
#pragma endregion
};