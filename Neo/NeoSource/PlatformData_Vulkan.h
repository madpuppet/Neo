#pragma once

struct TexturePlatformData
{
public:
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
};
TexturePlatformData* TexturePlatformData_Create(struct TextureAssetData* assetData);
void TexturePlatformData_Destroy(TexturePlatformData* platformData);

struct ShaderPlatformData
{
public:
	VkShaderModule shaderModule;
};
ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData);
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData);

struct MaterialPlatformData
{
public:

};
MaterialPlatformData* MaterialPlatformData_Create(struct MaterialAssetData* assetData);
void MaterialPlatformData_Destroy(MaterialPlatformData* platformData);

