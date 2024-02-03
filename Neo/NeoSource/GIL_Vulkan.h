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
#include "MathUtils.h"

const int MAX_FRAMES_IN_FLIGHT = 3;

struct UniformBufferObject {
    alignas(16) mat4x4 model;
    alignas(16) mat4x4 view;
    alignas(16) mat4x4 proj;
};

class GIL : public Module<GIL>
{
public:
	// main thread startup... creates window for message polling on main thread
	void StartupMainThread();

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

	// add model to render queue
	// must be called on render thread
	void RenderModel(class Model* model);

	void WaitTilInitialised() { m_vulkanInitialised.Wait(); }

	VkFormat FindVulkanFormat(TexturePixelFormat format) { return m_neoFormatToVulkanFormat[format]; }

	void SetModelMatrix(const mat4x4& modelMat);
	void SetViewMatrices(const mat4x4 &viewMat, const mat4x4 &projMat);
	void SetViewport(const rect &viewport, float minDepth, float maxDepth);
	void SetScissor(const rect &scissorRect);

	// TODO: should do a Platform Interface Layer for misc services not graphics related
	// show message box and return TRUE if user would like to break
	bool ShowMessageBox(const string& string);
	float GetJoystickAxis(int idx) { return Clamp(SDL_JoystickGetAxis(m_joystick, idx) / 32767.0f, -1.0f, 1.0f); }

protected:

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	SDL_Window* m_window;
	SDL_Joystick* m_joystick;
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
	VkCommandPool m_commandPool;

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
	void createCommandPool();
	void createDepthResources();
	void createFramebuffers();
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);
	void createTextureSampler();
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


	// these methods are used by external functions like Texture Platform creation
public:
	VkDevice Device() { return m_device; }
	VkCommandPool CommandPool() { return m_commandPool; }
	VkDescriptorPool DescriptorPool() { return m_descriptorPool; }
	std::vector<VkBuffer>& UniformBuffers() { return m_uniformBuffers; }
	VkSampler TextureSampler() { return m_textureSampler; }

	void createImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkRenderPass& GetRenderPass() { return m_renderPass; };
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
};


#endif // GRAPHICS_Vulkan


