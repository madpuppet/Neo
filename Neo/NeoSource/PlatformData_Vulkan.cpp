#include "Neo.h"
#include "PlatformData_Vulkan.h"
#include "Texture.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Material.h"
#include "StaticMesh.h"
#include "BitmapFont.h"

TexturePlatformData* TexturePlatformData_Create(TextureAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Texture, STR("PLATFORM DATA for TEXTURE: {}", assetData->name));

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

    gil.generateMipmaps(platformData->textureImage, fmt, assetData->width, assetData->height, 1);

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



VkShaderModule CreateShader(VkDevice device, MemBlock spv)
{
    VkShaderModule shaderModule = nullptr;

    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv.Size();
    createInfo.pCode = (u32*)spv.Mem();

    vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    return shaderModule;
}

PixelShaderPlatformData* PixelShaderPlatformData_Create(struct PixelShaderAssetData* assetData)
{
    PixelShaderPlatformData* platformData = new PixelShaderPlatformData;
    platformData->shaderModule = CreateShader(GIL::Instance().Device(), assetData->spvData);
    return platformData;
}

void PixelShaderPlatformData_Destroy(PixelShaderPlatformData* platformData)
{
    auto device = GIL::Instance().Device();
    vkDestroyShaderModule(device, platformData->shaderModule, nullptr);
}

VertexShaderPlatformData* VertexShaderPlatformData_Create(struct VertexShaderAssetData* assetData)
{
    VertexShaderPlatformData* platformData = new VertexShaderPlatformData;
    platformData->shaderModule = CreateShader(GIL::Instance().Device(), assetData->spvData);
    return platformData;
}

void VertexShaderPlatformData_Destroy(VertexShaderPlatformData* platformData)
{
    auto device = GIL::Instance().Device();
    vkDestroyShaderModule(device, platformData->shaderModule, nullptr);
}


MaterialPlatformData* MaterialPlatformData_Create(MaterialAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Material, STR("PLATFORM DATA for MATERIAL: {}", assetData->name));

    auto platformData = new MaterialPlatformData;
    auto& gil = GIL::Instance();

    auto vertShaderModule = assetData->vertexShader->GetPlatformData()->shaderModule;
    auto pixelShaderModule = assetData->pixelShader->GetPlatformData()->shaderModule;
    TexturePlatformData* texturePD = nullptr;
    for (auto uniform : assetData->uniforms)
    {
        if (uniform->type == UniformType_Texture)
        {
            auto uniformTex = (MaterialUniform_Texture*)uniform;
            texturePD = uniformTex->texture->GetPlatformData();
        }
    }

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(gil.Device(), &layoutInfo, nullptr, &platformData->descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = vertShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pixelShaderStageInfo{};
    pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pixelShaderStageInfo.module = pixelShaderModule;
    pixelShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, pixelShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode =  VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &platformData->descriptorSetLayout;     // TODO: this is hardcoded to uniform buffer + sampler atm. these are dependant on the shaders

    if (vkCreatePipelineLayout(gil.Device(), &pipelineLayoutInfo, nullptr, &platformData->pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = platformData->pipelineLayout;
    pipelineInfo.renderPass = gil.GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(gil.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &platformData->pipeline) != VK_SUCCESS) 
    {
        Error("Failed to create Graphics Pipeline for material");
    }

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, platformData->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = gil.DescriptorPool();
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    platformData->descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(gil.Device(), &allocInfo, platformData->descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = gil.UniformBuffers()[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texturePD->textureImageView;
        imageInfo.sampler = gil.TextureSampler();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = platformData->descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = platformData->descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(gil.Device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }


    return platformData;
}

void MaterialPlatformData_Destroy(MaterialPlatformData* platformData)
{
    delete platformData;
}

StaticMeshPlatformData* StaticMeshPlatformData_Create(struct StaticMeshAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Gfx, STR("PLATFORM DATA for StaticMesh: {}", assetData->name));

    auto& gil = GIL::Instance();
    auto platformData = new StaticMeshPlatformData;
    auto device = gil.Device();

    platformData->geomBuffer = gil.CreateGeometryBuffer(assetData->verts.data(), (u32)(sizeof(assetData->verts[0]) * assetData->verts.size()), assetData->indices.data(), (u32)(sizeof(assetData->indices[0]) * assetData->indices.size()));
    platformData->indiceCount = (int)assetData->indices.size();

    return platformData;
}

void StaticMeshPlatformData_Destroy(StaticMeshPlatformData* platformData)
{
}

BitmapFontPlatformData* BitmapFontPlatformData_Create(BitmapFontAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Gfx, STR("PLATFORM DATA for BitmapFont: {}", assetData->name));

    auto& gil = GIL::Instance();
    auto platformData = new BitmapFontPlatformData;
    auto device = gil.Device();

    return platformData;
}

void BitmapFontPlatformData_Destroy(BitmapFontPlatformData* platformData)
{
}





