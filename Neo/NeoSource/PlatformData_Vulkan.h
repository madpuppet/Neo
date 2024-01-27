#pragma once

struct TexturePlatformData
{
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
};
TexturePlatformData* TexturePlatformData_Create(struct TextureAssetData* assetData);
void TexturePlatformData_Destroy(TexturePlatformData* platformData);

struct ShaderPlatformData
{
	VkShaderModule shaderModule;
};
ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData);
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData);

struct MaterialPlatformData
{
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};
MaterialPlatformData* MaterialPlatformData_Create(struct MaterialAssetData* assetData);
void MaterialPlatformData_Destroy(MaterialPlatformData* platformData);

struct ModelPlatformData
{

};
ModelPlatformData* ModelPlatformData_Create(struct ModelAssetData* assetData);
void ModelPlatformData_Destroy(ModelPlatformData* platformData);
