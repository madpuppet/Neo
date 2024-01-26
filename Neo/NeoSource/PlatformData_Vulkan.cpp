#include "Neo.h"
#include "Texture.h"
#include "Shader.h"

#if NEW_CODE

TexturePlatformData* TexturePlatformData_Create(TextureAssetData* assetData)
{
    Log(STR("Create Texture Platform Data: {}", assetData->name));

    TexturePlatformData* platformData = new TexturePlatformData;

    auto& gil = GIL::Instance();

    VkDeviceSize imageSize = assetData->images[0].Size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    gil.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(gil.Device(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, assetData->images[0].Mem(), static_cast<size_t>(imageSize));
    vkUnmapMemory(gil.Device(), stagingBufferMemory);

    Assert(assetData->format != PixFmt_Undefined, "Undefined pixel format!");
    VkFormat fmt = gil.FindVulkanFormat(assetData->format);
    gil.createImage(assetData->width, assetData->height, 1, fmt, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, platformData->textureImage, platformData->textureImageMemory);
    gil.transitionImageLayout(platformData->textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
    gil.copyBufferToImage(stagingBuffer, platformData->textureImage, static_cast<uint32_t>(assetData->width), static_cast<uint32_t>(assetData->height));

    vkDestroyBuffer(gil.Device(), stagingBuffer, nullptr);
    vkFreeMemory(gil.Device(), stagingBufferMemory, nullptr);

    platformData->textureImageView = gil.createImageView(platformData->textureImage, fmt, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    return platformData;
};

void TexturePlatformData_Destroy(struct TexturePlatformData* platformData)
{
    auto& gil = GIL::Instance();
    auto device = gil.Device();
    vkDestroyImageView(device, platformData->textureImageView, nullptr);
    vkDestroyImage(device, platformData->textureImage, nullptr);
    vkFreeMemory(device, platformData->textureImageMemory, nullptr);
}

ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData)
{
    Log(STR("Create Shader Platform Data: {}", assetData->name));

    ShaderPlatformData* platformData = new ShaderPlatformData;
    auto device = GIL::Instance().Device();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = assetData->spvData.Size();
    createInfo.pCode = (u32*)assetData->spvData.Mem();

    if (vkCreateShaderModule(device, &createInfo, nullptr, &platformData->shaderModule) != VK_SUCCESS) 
    {
        Error(std::format("Failed to create shader module for shader: {}!", assetData->name));
    }

    return platformData;
}

void ShaderPlatformData_Destroy(ShaderPlatformData* platformData)
{
    auto device = GIL::Instance().Device();
    vkDestroyShaderModule(device, platformData->shaderModule, nullptr);
}

MaterialPlatformData* MaterialPlatformData_Create(struct MaterialAssetData* assetData)
{
    auto platformData = new MaterialPlatformData;
    return platformData;
}

void MaterialPlatformData_Destroy(MaterialPlatformData* platformData)
{
    delete platformData;
}

#else
TexturePlatformData* TexturePlatformData_Create(TextureAssetData* assetData) { return nullptr; }
void TexturePlatformData_Destroy(struct TexturePlatformData* platformData) {}
ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData) { return nullptr; }
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData) {}
MaterialPlatformData* MaterialPlatformData_Create(struct MaterialAssetData* assetData) { return nullptr; }
void MaterialPlatformData_Destroy(MaterialPlatformData* platformData) {}
#endif




