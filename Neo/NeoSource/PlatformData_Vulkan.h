#pragma once

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


struct MaterialPlatformData
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorSetLayout descriptorSetLayout;
	vector<VkDescriptorSet> descriptorSets;
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
