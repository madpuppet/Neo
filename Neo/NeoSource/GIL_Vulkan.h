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
#include "ShaderManager.h"
#include "RenderPass.h"

VkFormat VertexFormatToVk(VertexFormat vf);
string VertexFormatToString(VertexFormat vf);

// geometry buffers hold vertex & index data for rendering sets of geometry
struct NeoGeometryBuffer
{
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	u32 vertexBufferSize = 0;

	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
	u32 indexBufferSize = 0;
};

class GIL : public Module<GIL>
{
public:
	// startup procedures that need to run on the main thread... creates window for message polling on main thread
	void StartupMainThread();

	// initial basic global rendering systems
	// these run on the RenderThread
	void Initialize();

	// destroy all vulkan resources
	void Shutdown();

	// wait for fences before we start new command buffer
	void FrameWait();

	// start of a rendering frame (present last frame, prepare to build this frames queue)
	void BeginFrame();

	// execute queue for this frame 
	void EndFrame();

	// resize frame buffers
	void ResizeFrameBuffers(int width, int height);

	// graphics functions
	// create and destroy vertex&indice buffers for use with geometry rendering
	NeoGeometryBuffer* CreateGeometryBuffer(void* vertData, u32 bufferSize, void* indiceData, u32 indiceSize);
	void DestroyGeometryBuffer(NeoGeometryBuffer* buffer);
	void MapGeometryBufferMemory(NeoGeometryBuffer* buffer, void**vertexMem, void**indexMem);
	void FlushGeometryBufferMemory(NeoGeometryBuffer* buffer, u32 vertDataSize, u32 indexDataSize);
	void UnmapGeometryBufferMemory(NeoGeometryBuffer* buffer);

	// use material
	void BindMaterial(class Material* material, bool lines);

	// bind geometry buffers
	void BindGeometryBuffer(NeoGeometryBuffer* buffer);

	// render primitive
	void SetRenderPrimitiveType(PrimType primType);

	// render primitive
	void RenderPrimitive(u32 vertStart, u32 vertCount, u32 indexStart, u32 indexCount);

	// add model to render queue
	// must be called on render thread
	void RenderStaticMesh(class StaticMesh* mesh);

	// render an array of static mesh instances
	void RenderStaticMeshInstances(class StaticMesh* mesh, mat4x4 *ltw, u32 ltwCount);

	// update the entire memory for a single ubo instance
	void UpdateUBOInstance(UBOInfoInstance* uboInstance, void* uboMem, u32 uboSize, bool updateBoundMaterial);

	// update a single ubo instance member
	// set 'flush' to apply those changes if your not doing a series of updates
	void UpdateUBOInstanceMember(UBOInfoInstance* uboInstance, u32 memberOffset, const void* data, u32 datasize, bool flush);

	// override current graphics pipeline viewport
	void SetViewport(const rect &viewport, float minDepth, float maxDepth);

	// override current graphics pipeline scissor rectangle
	void SetScissor(const rect &scissorRect);

	// set the current render pass
	// transition all the textures to the appropriate layout
	void SetRenderPass(RenderPass *renderPass);

	// change layout of a texture
	void TransitionTexture(Texture *texture, TextureLayout currentLayout, TextureLayout newLayout);

	// query the current size of the frame buffer
	ivec2 GetFrameBufferSize() { return m_frameBufferSize; }

	// TODO: should do a Platform Interface Layer for misc services not graphics related
	// show message box and return TRUE if user would like to break
	bool ShowMessageBox(const string& string);
	float GetJoystickAxis(int idx);

protected:
	void WaitForMemory();

#if PROFILING_ENABLED
	static const int MaxProfileTimestamps = 2000;
	u32 m_profile_idx = 0;
	u64 m_profile_timestamps[MaxProfileTimestamps];
#endif

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	SDL_Window* m_window;
	SDL_Joystick* m_joystick;
	ivec2 m_frameBufferSize{ 1280,720 };

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
	VkFormat m_depthFormat;
	VkExtent2D m_swapChainExtent;
	vector<VkImageView> m_swapChainImageViews;
	vector<VkFramebuffer> m_swapChainFramebuffers;
	u32 m_frameSwapImage;

#if PROFILING_ENABLED
	static const int MaxQueries = 5000;
	VkQueryPool m_queryPool;
	u64 m_queryResults[MaxQueries];
	u32 m_queryIdx = 0;
#endif

	hashtable<TexturePixelFormat, VkFormat> m_neoFormatToVulkanFormat;

	// all these probably get replaced by per-resource data once I replace more systems with resources like renderTargets, shaders and samplers
	VkRenderPass m_renderPass;
	VkCommandPool m_commandPool;

	RenderPass* m_activeRenderPass = nullptr;

	VkImage m_depthImage;
	VkDeviceMemory m_depthImageMemory;
	VkImageView m_depthImageView;

	Material* m_boundMaterial;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	u32 m_currentFrame = 0;		// this toggles 0 <-> 1  for driving double buffered structures
	u32 m_uniqueFrameidx = 0;	// this just increments every frame so we can tell if a resource has marked as used this frame yet
	bool m_framebufferResized = false;

	static const int MaxDynamicUniformBufferMemory = 1024 * 1024;
	// shared memory used for all dynamic uniform buffers each frame
	array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> m_dynamicUniformBufferMemory;
	// mapped dynamic uniform buffer memory
	array<void*, MAX_FRAMES_IN_FLIGHT> m_dynamicUniformBufferMemoryMapped;
	// how much has been allocated this frame
	u32 m_dynamicUniformBufferMemoryUsed = 0;
	// intialize the dynamic memory
	void createUniformBufferDynamicMemory();
	// allocate a 64byte aligned block of uniform buffer memory from the shared pool for the current frame
	u32 AllocateDynamicUniformBufferMemory(u32 size);


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

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void createDescriptorPool();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapChain();
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

	// these methods expose Vulkan internals and are used by PlatformData functions like Texture Platform creation
public:
	VkDevice Device() { return m_device; }
	VkCommandPool CommandPool() { return m_commandPool; }
	VkDescriptorPool DescriptorPool() { return m_descriptorPool; }

	// these used by RenderPass
	VkFormat FindVulkanFormat(TexturePixelFormat format) { return m_neoFormatToVulkanFormat[format]; }
	VkFormat GetSwapChainImageFormat() { return m_swapChainImageFormat; }
	VkFormat GetDepthFormat() { return m_depthFormat; }
	VkImageView GetDepthBufferImageView() { return m_depthImageView; }
	void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	// these used for UBO buffer creation
	// dynamic UBOs just share shader memory over a frame and must copy over the entire UBO memory on any change,
	//   whereas static UBOs just double buffer a block of memory and assume it stays static during a single frame
	void createUniformBuffer(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT>& memory,
		array<void*, MAX_FRAMES_IN_FLIGHT>& mapped, u32 size);
	void createUniformBufferDynamic(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<void*, MAX_FRAMES_IN_FLIGHT>& mapped);

	// these helper functions all used for texture platform data creations
	bool createTextureSampler(VkFilter minFilter, VkFilter maxFilter, VkSamplerMipmapMode mipMapFilter, VkSamplerAddressMode addressingU,
		VkSamplerAddressMode addressingV, VkCompareOp compareOp, VkSampler& sampler);
	void createImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createDynamicBuffer(VkDeviceSize bufferSize, VkDeviceSize memorySize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyMemoryToBuffer(VkDeviceMemory bufferMemory, void* memory, size_t size);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	VkRenderPass& GetRenderPass() { return m_renderPass; };
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

#if PROFILING_ENABLED
	void createTimeQueries();
	u32 AddGpuTimeQuery();
	void GetGpuTimeQueryResults(u64*& buffer, int& count);
#endif
};


#endif // GRAPHICS_Vulkan


