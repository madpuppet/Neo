#pragma once

struct UBOInfoInstance;
const int MAX_FRAMES_IN_FLIGHT = 2;

struct TexturePlatformData
{
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
};
TexturePlatformData* TexturePlatformData_Create(struct TextureAssetData* assetData);
void TexturePlatformData_Destroy(TexturePlatformData* platformData);

struct VertexShaderPlatformData
{
	VkShaderModule shaderModule;
};
VertexShaderPlatformData* VertexShaderPlatformData_Create(struct VertexShaderAssetData* assetData);
void VertexShaderPlatformData_Destroy(VertexShaderPlatformData* platformData);

struct PixelShaderPlatformData
{
	VkShaderModule shaderModule;
};
PixelShaderPlatformData* PixelShaderPlatformData_Create(struct PixelShaderAssetData* assetData);
void PixelShaderPlatformData_Destroy(PixelShaderPlatformData* platformData);

struct ShaderPlatformData
{
	VkShaderModule vertShaderModule;
	VkShaderModule fragShaderModule;
};
ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData);
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData);

struct MaterialPlatformData
{
	VkPipelineLayout pipelineLayout;
	VkPipeline polygonPipeline;
	VkPipeline linePipeline;

	// we'll do the layouts per material so we can mark some materials as being static UBOs instead of dynamic UBOs
	static const int MaxSets = 4;
	vector<VkDescriptorSetLayout> dsSetLayouts;
	u32 setMapping[MaxSets]{};		// maps shader 'set' to set index

	vector<VkDescriptorSet> descriptorSets[MAX_FRAMES_IN_FLIGHT];

	vector<UBOInfoInstance*> uboInstances;		// all ubo instances flattened (shader ones, with material overrides)

//	vector<u32> dynamicOffsets[MAX_FRAMES_IN_FLIGHT];
//	vector<u32> idxToDynamicOffset;
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

	bool isDynamic;		// dynamic - means we are bound to the frame dynamic buffer and step through slices in that on each use
	u32 size;			// size of buffer (adjusted to alignment size)
	u32 memOffset;		// current memoffset into shared memory
};
UniformBufferPlatformData* UniformBufferPlatformData_Create(const struct UBOInfo &uboInfo, bool dynamic);
void UniformBufferPlatformData_Destroy(UniformBufferPlatformData* platformData);

