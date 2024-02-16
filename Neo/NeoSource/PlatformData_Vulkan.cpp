#include "Neo.h"
#include "PlatformData_Vulkan.h"
#include "Texture.h"
#include "VertexShader.h"
#include "PixelShader.h"
#include "Material.h"
#include "StaticMesh.h"
#include "ShaderManager.h"

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


MaterialSampler* FindSampler(MaterialAssetData* assetData, const string& name)
{
    for (auto sampler : assetData->samplers)
    {
        if (StringEqual(sampler->samplerName, name))
            return sampler;
    }
    return nullptr;
}

MaterialBufferObject* FindMBO(MaterialAssetData* assetData, const string& name)
{
    for (auto mbo : assetData->buffers)
    {
        if (StringEqual(mbo->uboInstance->ubo->structName, name))
            return mbo;
    }
    return nullptr;
}

MaterialPlatformData* MaterialPlatformData_Create(MaterialAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Material, STR("PLATFORM DATA for MATERIAL: {}", assetData->name));

    auto platformData = new MaterialPlatformData;
    auto& gil = GIL::Instance();

    auto shaderPD = assetData->shader->GetPlatformData();
    auto shaderAD = assetData->shader->GetAssetData();

    VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
    vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderStageInfo.module = shaderPD->vertShaderModule;
    vertexShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo pixelShaderStageInfo{};
    pixelShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pixelShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    pixelShaderStageInfo.module = shaderPD->fragShaderModule;
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
    switch (assetData->cullMode)
    {
        case MaterialCullMode_None:	    			// don't cull any triangles
            rasterizer.cullMode = VK_CULL_MODE_NONE;
            break;
        case MaterialCullMode_Front:				// cull front facing triangles
            rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
            break;
        case MaterialCullMode_Back: 				// cull back facing triangles
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            break;
    };
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = assetData->zread ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = assetData->zwrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    switch (assetData->blendMode)
    {
        case MaterialBlendMode_Opaque:			// use src color, no blending
            colorBlendAttachment.blendEnable = VK_FALSE;
            break;
        case MaterialBlendMode_Blend:           // normal alpha blending
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case MaterialBlendMode_Alpha:			// pre-multiplied alpha (one,inv_alpha)
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case MaterialBlendMode_Additive:			// add src color to destination
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
            break;
        case MaterialBlendMode_Subtractive:		// subtract src color from destination
            colorBlendAttachment.blendEnable = VK_TRUE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;            
            break;
    }

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
    pipelineLayoutInfo.setLayoutCount = (u32)shaderPD->dsSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = shaderPD->dsSetLayouts.data();

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

    if (vkCreateGraphicsPipelines(gil.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &platformData->polygonPipeline) != VK_SUCCESS) 
    {
        Error("Failed to create Graphics Pipeline for material");
    }

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    if (vkCreateGraphicsPipelines(gil.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &platformData->linePipeline) != VK_SUCCESS)
    {
        Error("Failed to create Graphics Pipeline for material");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = gil.DescriptorPool();
    allocInfo.descriptorSetCount = (u32)shaderPD->dsSetLayouts.size();
    allocInfo.pSetLayouts = shaderPD->dsSetLayouts.data();
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        platformData->descriptorSets->resize(allocInfo.descriptorSetCount);
        if (vkAllocateDescriptorSets(gil.Device(), &allocInfo, platformData->descriptorSets->data()) != VK_SUCCESS)
            Error("failed to allocate descriptor sets!");
    }

    auto& sm = ShaderManager::Instance();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vector<VkWriteDescriptorSet> descriptorWrites;
        for (auto& sro : shaderAD->SROs)
        {
            VkWriteDescriptorSet wds{};
            wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            wds.dstSet = platformData->descriptorSets[i][shaderPD->setMapping[sro.set]];
            wds.dstBinding = sro.binding;
            wds.dstArrayElement = 0;
            wds.descriptorCount = 1;

            switch (sro.type)
            {
                case SROType_UBO:
                {
                    VkDescriptorBufferInfo bufferInfo{};
                    auto mbo = FindMBO(assetData, sro.name);
                    if (mbo)
                    {
                        bufferInfo.buffer = mbo->uboInstance->platformData->buffer[i];
                        wds.descriptorType = mbo->uboInstance->isDynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    }

                    auto uboInfo = sm.FindUBO(sro.name);
                    bufferInfo.buffer = uboInfo->platformData->buffer[i];
                    bufferInfo.offset = 0;
                    bufferInfo.range = uboInfo->size;
                    wds.pBufferInfo = &bufferInfo;
                    wds.descriptorType = (sro.type == SROType_UBO) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                }
                break;

                case SROType_Sampler:
                {
                    VkDescriptorImageInfo imageInfo{};
                    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    auto sampler = FindSampler(assetData, sro.name);
                    Assert(sampler, STR("Cannot find texture '{}' in material '{}' referenced by the shader", sro.name, assetData->name));
                    imageInfo.imageView = sampler->texture->GetPlatformData()->textureImageView;
                    VkFilter minFilter = VK_FILTER_NEAREST;
                    VkFilter maxFilter = VK_FILTER_NEAREST;
                    VkSamplerMipmapMode mipMapFilter = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
                    gil.createTextureSampler(minFilter, maxFilter, mipMapFilter, addressMode, compareOp, imageInfo.sampler);
                    wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    wds.pImageInfo = &imageInfo;
                }
                break;
            }

            descriptorWrites.push_back(wds);
        }
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

ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData)
{
    ShaderPlatformData* platformData = new ShaderPlatformData;
    platformData->vertShaderModule = CreateShader(GIL::Instance().Device(), assetData->vertSPVData);
    platformData->fragShaderModule = CreateShader(GIL::Instance().Device(), assetData->fragSPVData);

    // descriptor layouts for each set..
    vector<VkDescriptorSetLayoutBinding> layoutBindings[ShaderPlatformData::MaxSets];
    for (auto& sro : assetData->SROs)
    {
        VkDescriptorSetLayoutBinding dslBinding{};
        dslBinding.binding = sro.binding;
        dslBinding.descriptorCount = 1;
        dslBinding.pImmutableSamplers = nullptr;
        switch (sro.type)
        {
            case SROType_UBO:
                dslBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case SROType_Sampler:
                dslBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case SROType_UBOD:
                dslBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                break;
            case SROType_SBO:
                dslBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
        }
        dslBinding.stageFlags = 0;
        if (sro.stageMask & SROStage_Vertex)
            dslBinding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (sro.stageMask & SROStage_Fragment)
            dslBinding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[sro.set].push_back(dslBinding);
    }

    auto& gil = GIL::Instance();
    for (int i = 0; i < ShaderPlatformData::MaxSets; i++)
    {
        if (!layoutBindings[i].empty())
        {
            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings[i].size());
            layoutInfo.pBindings = layoutBindings[i].data();
            VkDescriptorSetLayout dsl;
            if (vkCreateDescriptorSetLayout(gil.Device(), &layoutInfo, nullptr, &dsl) != VK_SUCCESS)
                Error("failed to create descriptor set layout!");
            platformData->setMapping[i] = (u32)platformData->dsSetLayouts.size();
            platformData->dsSetLayouts.push_back(dsl);
        }
    }
    return platformData;
}
void ShaderPlatformData_Destroy(ShaderPlatformData* platformData)
{
    auto device = GIL::Instance().Device();
    vkDestroyShaderModule(device, platformData->vertShaderModule, nullptr);
    vkDestroyShaderModule(device, platformData->fragShaderModule, nullptr);
}

UniformBufferPlatformData* UniformBufferPlatformData_Create(const UBOInfo &uboInfo, bool dynamic)
{
    auto platformData = new UniformBufferPlatformData;
    if (dynamic)
    {
        GIL::Instance().createUniformBufferDynamic(platformData->buffer, platformData->memoryMapped);
    }
    else
    {
        GIL::Instance().createUniformBuffer(platformData->buffer, platformData->memory, platformData->memoryMapped, uboInfo.size);
    }
    return platformData;
}
void UniformBufferPlatformData_Destroy(UniformBufferPlatformData* platformData)
{
}


