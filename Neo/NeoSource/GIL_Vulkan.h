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

struct Vertex {
	glm::vec3 pos;
	glm::vec2 texCoord;
	u32 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
		attributeDescriptions[2].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};
template<> struct std::hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const
	{
		return ((std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<u32>()(vertex.color) << 1)) >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};


struct UniformBufferObject {
    alignas(16) mat4x4 model;
    alignas(16) mat4x4 view;
    alignas(16) mat4x4 proj;
	alignas(16) vec4 blend;
};

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
	void Startup();

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

	void WaitTilInitialised() { m_vulkanInitialised.Wait(); }

	VkFormat FindVulkanFormat(TexturePixelFormat format) { return m_neoFormatToVulkanFormat[format]; }

	void SetModelMatrix(const mat4x4& modelMat);

	void SetAndBindModelMatrix(const mat4x4& modelMat);

	void SetViewMatrices(const mat4x4 &viewMat, const mat4x4 &projMat, const mat4x4& orthoMat);
	void SetMaterialBlendColor(const vec4& blendColor);
	void SetViewport(const rect &viewport, float minDepth, float maxDepth);
	void SetScissor(const rect &scissorRect);
	ivec2 GetFrameBufferSize() { return m_frameBufferSize; }

	// TODO: should do a Platform Interface Layer for misc services not graphics related
	// show message box and return TRUE if user would like to break
	bool ShowMessageBox(const string& string);
	float GetJoystickAxis(int idx);

protected:
	void WaitForMemory();

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	SDL_Window* m_window;
	SDL_Joystick* m_joystick;
	Semaphore m_vulkanInitialised;
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

	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	Material* m_boundMaterial;
	u32 m_currModelDynamicOffset;
	u32 m_nextModelDynamicOffset;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;
	u32 m_currentFrame = 0;
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
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();
	void recreateSwapChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	u32 findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties);

	// these methods are used by external functions like Texture Platform creation
public:
	VkDevice Device() { return m_device; }
	VkCommandPool CommandPool() { return m_commandPool; }
	VkDescriptorPool DescriptorPool() { return m_descriptorPool; }

	void createUniformBuffer(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT>& memory,
		array<void*, MAX_FRAMES_IN_FLIGHT>& mapped, u32 size);
	void createUniformBufferDynamic(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<void*, MAX_FRAMES_IN_FLIGHT>& mapped);

	bool createTextureSampler(VkFilter minFilter, VkFilter maxFilter, VkSamplerMipmapMode mipMapFilter, VkSamplerAddressMode addressing, VkCompareOp compareOp, VkSampler& sampler);
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
};


#endif // GRAPHICS_Vulkan


