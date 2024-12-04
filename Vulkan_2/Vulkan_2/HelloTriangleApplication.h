#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <vector>
#include <optional>
#include <fstream>
#include "Constants.h"

class Mesh;

static std::vector<char> readFile(const std::string& fileName)
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

	bool isComplete()
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
	void run();

private: // Run functions
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

private:
	void createInstance();
	void createLogicalDevice();
	void createSurface();
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createSwapChain();
	void createImageViews();
	void createGraphicsPipeline();
	void createRenderPass();
	void createFrameBuffers();
	void createCommandPools();
	void createVertexBuffer();
	void createCommandBuffer();
	void createSyncObjects();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

private: // helper functions
	// returns QueueFamilyIndices with graphics and present queues support
	QueueFamilyIndices FindQueueFamily(VkPhysicalDevice device);
	// explicitly looks for a specific queue family flag
	QueueFamilyIndices FindQueueFamily(VkPhysicalDevice device, uint32_t queueFamilyFlag);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSurfaceFormat(std::vector<VkSurfaceFormatKHR> surfaceFormats);
	VkPresentModeKHR ChoosePresentMode(std::vector<VkPresentModeKHR> presentModes);
	VkExtent2D ChooseSwapChainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkShaderModule CreateShaderModule(const std::vector<char>& shaderCode);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

private:
	void recordCommandBuffer(VkCommandBuffer commmandBuffer, uint32_t swapChainImageIndex);
	void drawFrame();
	void cleanupSwapChain();
	void recreateSwapChain();

private: // util functions
	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkRequiredSupportedExtensionsPresent(const char** extensions, uint32_t extensionCount);
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions(uint32_t& extensionsCount);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

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

	uint32_t _currentFrame = 0;

	std::vector<VkSemaphore> _imageReadySemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;

	bool _bFrameBufferResized = false;


#pragma region Move this to Component System
	Mesh* _mesh;
#pragma endregion
};