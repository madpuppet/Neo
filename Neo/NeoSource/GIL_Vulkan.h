#pragma once

#if defined(GRAPHICS_Vulkan)

// Graphics Interface Layer (GIL) for Windows
//
// This is a vulkan layer to run on windows platform - abstracts all vulkan interfaces from the rest of the engine
// generally all these functions should only be running on the GraphicsInterface thread
//
// Note: I may make this a vulkan layer if it turns out vulkan compatibilty between multiple platforms is nearly 100%
//       in which case I'll add a GIL_Vulkan header/class and have multiple platforms just include that

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <optional>
#include "PlatformData_Vulkan.h"
#include "Thread.h"
#include "Texture.h"

#if NEW_CODE
const int MAX_FRAMES_IN_FLIGHT = 2;
#endif

class GIL : public Module<GIL>
{

#if NEW_CODE

public:
	// initial basic global rendering systems
	void Startup();

	// destroy all vulkan resources
	void Shutdown();

	// start of a rendering frame (present last frame, prepare to build this frames queue)
	void BeginFrame();

	// execute queue for this frame 
	void EndFrame();

	// graphics functions
	void CreateRenderTarget();  // texture for use in render target state
	void CreateRenderTargetState();		// color, depth, cullmode
	void CreateUniformBuffer();
	void CreateSampler();
	void CreateShader();
	void CreateRenderState(); // sets up shaders, color blendings, msaa, 
	void CreateVertexAttributeBinding();

	void DrawTestFrame();

	void WaitTilInitialised() { m_vulkanInitialised.Wait(); }

	VkFormat FindVulkanFormat(TexturePixelFormat format) { return m_neoFormatToVulkanFormat[format]; }

protected:

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	SDL_Window* m_window;
	Semaphore m_vulkanInitialised;

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debugMessenger;
	VkSurfaceKHR m_surface;
	VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	VkDevice m_device;
	VkQueue m_graphicsQueue;
	VkQueue m_presentQueue;

	VkSwapchainKHR m_swapChain;
	vector<VkImage> m_swapChainImages;
	VkFormat m_swapChainImageFormat;
	VkExtent2D m_swapChainExtent;
	vector<VkImageView> m_swapChainImageViews;
	vector<VkFramebuffer> m_swapChainFramebuffers;
	u32 m_frameSwapImage;

	hashtable<TexturePixelFormat, VkFormat> m_neoFormatToVulkanFormat;

	// all these probably get replaced by per-resource data once I replace more systems with resources like renderTargets, shaders and samplers
	VkRenderPass m_renderPass;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;

	VkCommandPool m_commandPool;

	VkImage m_colorImage;
	VkDeviceMemory m_colorImageMemory;
	VkImageView m_colorImageView;

	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;

	VkSampler m_textureSampler;
	VkImageView m_textureImageView;

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	std::vector<VkBuffer> m_uniformBuffers;
	std::vector<VkDeviceMemory> m_uniformBuffersMemory;
	std::vector<void*> m_uniformBuffersMapped;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSets;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	u32 m_currentFrame = 0;
	bool m_framebufferResized = false;

	struct QueueFamilyIndices
	{
		std::optional<u32> graphicsFamily;
		std::optional<u32> presentFamily;
		bool isComplete() 
		{
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		vector<VkSurfaceFormatKHR> formats;
		vector<VkPresentModeKHR> presentModes;
	};

	void createInstance();
	void createSurface();
	bool checkValidationLayerSupport();
	vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	void pickPhysicalDevice();
	void createLogicalDevice();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	void createSwapChain();
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void createImageViews();
	void createRenderPass();
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	void cleanupSwapChain();
	void createFormatMappings();

	// these will change when we support samples, shaders, uniform buffers, texture, materials
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createColorResources();
	void createDepthResources();
	void createFramebuffers();
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	void createTextureSampler();
	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapChain();
	void updateUniformBuffer(uint32_t currentImage);
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

#else //!NEW_CODE
public:
	void Startup() {}
	void Shutdown() {}
	void BeginFrame() {}
	void EndFrame() {}
	void WaitTilInitialised() {}
#endif

	// these methods are used by external functions like Texture Platform creation
public:
#if NEW_CODE
	VkDevice Device() { return m_device; }
	void createImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
#endif
};


#endif // GRAPHICS_Vulkan


