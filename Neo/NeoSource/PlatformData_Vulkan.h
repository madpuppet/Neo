#pragma once

struct UBOInfoInstance;
const int MAX_FRAMES_IN_FLIGHT = 2;

struct TexturePlatformData
{
	bool isDepth = false;
	VkImage textureImage = nullptr;
	VkDeviceMemory textureImageMemory = nullptr;
	VkImageView textureImageView = nullptr;
};
TexturePlatformData* TexturePlatformData_Create(struct TextureAssetData* assetData);
void TexturePlatformData_Destroy(TexturePlatformData* platformData);

struct VertexShaderPlatformData
{
	VkShaderModule shaderModule = nullptr;
};
VertexShaderPlatformData* VertexShaderPlatformData_Create(struct VertexShaderAssetData* assetData);
void VertexShaderPlatformData_Destroy(VertexShaderPlatformData* platformData);

struct PixelShaderPlatformData
{
	VkShaderModule shaderModule = nullptr;
};
PixelShaderPlatformData* PixelShaderPlatformData_Create(struct PixelShaderAssetData* assetData);
void PixelShaderPlatformData_Destroy(PixelShaderPlatformData* platformData);

struct ShaderPlatformData
{
	VkShaderModule vertShaderModule = nullptr;
	VkShaderModule fragShaderModule = nullptr;
};
ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData);
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData);

struct MaterialPlatformData
{
	VkPipelineLayout pipelineLayout = nullptr;
	VkPipeline polygonPipeline = nullptr;
	VkPipeline linePipeline = nullptr;

	// we'll do the layouts per material so we can mark some materials as being static UBOs instead of dynamic UBOs
	static const int MaxSets = 4;
	vector<VkDescriptorSetLayout> dsSetLayouts;
	u32 setMapping[MaxSets]{};		// maps shader 'set' to set index

	vector<VkDescriptorSet> descriptorSets[MAX_FRAMES_IN_FLIGHT];

	// all ubo instances flattened (set, instance) pairs
	vector<std::pair<int, UBOInfoInstance*>> uboInstances;		
};
MaterialPlatformData* MaterialPlatformData_Create(struct MaterialAssetData* assetData);
void MaterialPlatformData_Destroy(MaterialPlatformData* platformData);

struct StaticMeshPlatformData
{
	struct NeoGeometryBuffer* geomBuffer = nullptr;
	u32 indiceCount = 0;
};
StaticMeshPlatformData* StaticMeshPlatformData_Create(struct StaticMeshAssetData* assetData);
void StaticMeshPlatformData_Destroy(StaticMeshPlatformData* platformData);

struct UniformBufferPlatformData
{
	array<VkBuffer, MAX_FRAMES_IN_FLIGHT> buffer;
	array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> memory;
	array<void*, MAX_FRAMES_IN_FLIGHT> memoryMapped;

	bool isDynamic=true;		// dynamic - means we are bound to the frame dynamic buffer and step through slices in that on each use
	u32 size=0;			        // size of buffer (adjusted to alignment size)
	u32 memOffset=-1;	        // current memoffset into shared memory - set to -1 so we can see if a buffer is used before initialising
	u32 frameID=-1;				// dynamic - if frameID is out of date, we need to allocate an offset and copy 'data' from below
	void* data=nullptr;			// current state of UBO data mirrored for dynamic buffers
};
UniformBufferPlatformData* UniformBufferPlatformData_Create(const struct UBOInfo &uboInfo, bool dynamic);
void UniformBufferPlatformData_Destroy(UniformBufferPlatformData* platformData);

struct IADPlatformData
{
	vector<VkVertexInputBindingDescription> bindingDescriptions;
	vector<VkVertexInputAttributeDescription> attributeDescriptions;
};
IADPlatformData* IADPlatformData_Create(struct InputAttributesDescription *iad);

struct RenderPassPlatformData
{
	bool useSwapChain = false;
	VkRenderPass renderPass;
	VkFramebuffer frameBuffer;
	vector<VkClearValue> clearValues;
};
RenderPassPlatformData *RenderPassPlatformData_Create(struct RenderPassAssetData* assetData);

