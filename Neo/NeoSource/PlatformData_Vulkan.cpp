#include "Neo.h"
#include "PlatformData_Vulkan.h"
#include "Texture.h"
#include "Material.h"
#include "StaticMesh.h"
#include "ShaderManager.h"
#include "RenderPass.h"

VkSamplerAddressMode NeoAddressModeToVk[] = { VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_REPEAT };
VkCompareOp NeoCompareOpToVk[] = { VK_COMPARE_OP_NEVER, VK_COMPARE_OP_GREATER_OR_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL };

TexturePlatformData* TexturePlatformData_Create(TextureAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Texture, STR("PLATFORM DATA for TEXTURE: {}", assetData->name));

    TexturePlatformData* platformData = new TexturePlatformData;

    auto& gil = GIL::Instance();

    Assert(assetData->format != PixFmt_Undefined, "Undefined pixel format!");
    VkFormat fmt = gil.FindVulkanFormat(assetData->format);
    platformData->isDepth = (assetData->format == PixFmt_D32_SFLOAT || assetData->format == PixFmt_D24_UNORM_S8_UINT);

    int usageBit = platformData->isDepth ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    int aspectBit = platformData->isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    if (assetData->format == PixFmt_D24_UNORM_S8_UINT)
        aspectBit |= VK_IMAGE_ASPECT_STENCIL_BIT;

    if (assetData->isRenderTarget)
    {
        gil.createImage(assetData->width, assetData->height, 1, fmt,
            VK_IMAGE_TILING_OPTIMAL, usageBit | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, platformData->textureImage, platformData->textureImageMemory);

        // Create image view
        VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewInfo.image = platformData->textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = fmt;
        viewInfo.subresourceRange.aspectMask = aspectBit;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(gil.Device(), &viewInfo, nullptr, &platformData->textureImageView);

        gil.transitionImageLayout(platformData->textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);
    }
    else
    {
        VkDeviceSize imageSize = assetData->images[0].Size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        gil.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(gil.Device(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, assetData->images[0].Mem(), static_cast<size_t>(imageSize));
        vkUnmapMemory(gil.Device(), stagingBufferMemory);

        gil.createImage(assetData->width, assetData->height, 1, fmt, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, platformData->textureImage, platformData->textureImageMemory);
        gil.transitionImageLayout(platformData->textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
        gil.copyBufferToImage(stagingBuffer, platformData->textureImage, static_cast<uint32_t>(assetData->width), static_cast<uint32_t>(assetData->height));

        vkDestroyBuffer(gil.Device(), stagingBuffer, nullptr);
        vkFreeMemory(gil.Device(), stagingBufferMemory, nullptr);

        gil.generateMipmaps(platformData->textureImage, fmt, assetData->width, assetData->height, 1);
        platformData->textureImageView = gil.createImageView(platformData->textureImage, fmt, aspectBit, 1);
    }

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

MaterialSampler* FindSampler(MaterialRenderPassInfo* mrpi, const string& name)
{
    for (auto sampler : mrpi->samplers)
    {
        if (StringEqual(sampler->samplerName, name))
            return sampler;
    }
    return nullptr;
}

MaterialBufferObject* FindMBO(MaterialRenderPassInfo* mrpi, const string& name)
{
    for (auto mbo : mrpi->buffers)
    {
        if (StringEqual(mbo->uboInstance->ubo->structName, name))
            return mbo;
    }
    return nullptr;
}

bool IsMBODynamic(MaterialRenderPassInfo* mrpi, const string& name)
{
    for (auto mbo : mrpi->buffers)
    {
        if (StringEqual(mbo->uboInstance->ubo->structName, name))
            return mbo->uboInstance->isDynamic;
    }
    return true;
}

MaterialPlatformData* MaterialPlatformData_Create(MaterialAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    LOG(Material, STR("PLATFORM DATA for MATERIAL: {}", assetData->name));

    auto platformData = new MaterialPlatformData;
    auto& gil = GIL::Instance();

    for (auto mrpi : assetData->renderPasses)
    {
        auto rp = new MaterialPlatformRenderPassData;
        platformData->renderPasses.push_back(rp);

        auto shaderPD = mrpi->shader->GetPlatformData();
        auto shaderAD = mrpi->shader->GetAssetData();

        // create platform data for mbo's
        for (auto mbo : mrpi->buffers)
        {
            auto pd = UniformBufferPlatformData_Create(*mbo->uboInstance->ubo, mbo->uboInstance->isDynamic);
            mbo->uboInstance->platformData = pd;
            if (mbo->uboInstance->isDynamic)
            {
                mbo->uboInstance->platformData->data = new u8[mbo->uboInstance->ubo->size];
                memset(mbo->uboInstance->platformData->data, 0, mbo->uboInstance->ubo->size);
                for (auto& uniform : mbo->uniforms)
                {
                    memcpy((u8*)mbo->uboInstance->platformData->data + uniform.uboMember->offset, uniform.data, uniform.uboMember->datasize);
                }
            }
            else
            {
                for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
                {
                    memset(pd->memoryMapped[i], 0, mbo->uboInstance->ubo->size);
                    for (auto& uniform : mbo->uniforms)
                    {
                        memcpy((u8*)pd->memoryMapped[i] + uniform.uboMember->offset, uniform.data, uniform.uboMember->datasize);
                    }
                }
            }
        }

        // flatten out ubo instances
        for (auto sro : shaderAD->SROs)
        {
            if (sro.type == SROType_UBO)
            {
                // check if there is a material version
                UBOInfoInstance* uboInstance = nullptr;
                bool dynamic = true;
                for (auto mbo : mrpi->buffers)
                {
                    if (StringEqual(sro.name, mbo->uboInstance->ubo->structName))
                    {
                        uboInstance = mbo->uboInstance;
                    }
                }
                if (!uboInstance)
                {
                    auto ubo = ShaderManager::Instance().FindUBO(sro.name);
                    Assert(ubo, STR("Unable to find UBO {} for material {}", sro.name, assetData->name));
                    uboInstance = ubo->dynamicInstance;
                }
                rp->uboInstances.push_back(std::pair<int, UBOInfoInstance*>(sro.set, uboInstance));
            }
        }

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

        auto& bindingDescriptions = shaderAD->iad->platformData->bindingDescriptions;
        auto& attributeDescriptions = shaderAD->iad->platformData->attributeDescriptions;

        vertexInputInfo.vertexBindingDescriptionCount = (u32)bindingDescriptions.size();
        vertexInputInfo.vertexAttributeDescriptionCount = (u32)attributeDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        auto renderPassPD = mrpi->renderPass->GetPlatformData();
        auto renderPassAD = mrpi->renderPass->GetAssetData();
        ivec2 size = (renderPassPD->useSwapChain) ? gil.GetSwapChainImageSize() : renderPassAD->size;
        VkViewport vp{};
        vp.x = renderPassAD->viewportRect.x * size.x;
        vp.y = renderPassAD->viewportRect.y * size.x;
        vp.width = renderPassAD->viewportRect.w * size.x;
        vp.height = renderPassAD->viewportRect.h * size.y;
        vp.minDepth = 0;
        vp.maxDepth = 1;
        viewportState.pViewports = &vp;

        VkRect2D scissor{ (i32)(renderPassAD->scissorRect.x * size.x), (i32)(renderPassAD->scissorRect.y * size.y),
            (u32)(renderPassAD->scissorRect.w * size.x), (u32)(renderPassAD->scissorRect.h * size.y) };
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        switch (mrpi->cullMode)
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
        depthStencil.depthTestEnable = mrpi->zread ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = mrpi->zwrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        switch (mrpi->blendMode)
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
            VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // descriptor layouts for each set..
        vector<VkDescriptorSetLayoutBinding> layoutBindings[MaterialPlatformRenderPassData::MaxSets];
        for (auto& sro : shaderAD->SROs)
        {
            VkDescriptorSetLayoutBinding dslBinding{};
            dslBinding.binding = sro.binding;
            dslBinding.descriptorCount = 1;
            dslBinding.pImmutableSamplers = nullptr;
            switch (sro.type)
            {
                case SROType_UBO:
                    dslBinding.descriptorType = IsMBODynamic(mrpi, sro.name) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    break;
                case SROType_Sampler:
                    dslBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    break;
            }
            dslBinding.stageFlags = 0;
            if (sro.stageMask & SROStage_Vertex)
                dslBinding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
            if (sro.stageMask & SROStage_Fragment)
                dslBinding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            layoutBindings[sro.set].push_back(dslBinding);
        }

        for (int i = 0; i < MaterialPlatformRenderPassData::MaxSets; i++)
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
                rp->setMapping[i] = (u32)rp->dsSetLayouts.size();
                rp->dsSetLayouts.push_back(dsl);
            }
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = (u32)rp->dsSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = rp->dsSetLayouts.data();

        if (vkCreatePipelineLayout(gil.Device(), &pipelineLayoutInfo, nullptr, &rp->pipelineLayout) != VK_SUCCESS) {
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
        pipelineInfo.layout = rp->pipelineLayout;
        pipelineInfo.renderPass = mrpi->renderPass->GetPlatformData()->renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(gil.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rp->polygonPipeline) != VK_SUCCESS)
        {
            Error("Failed to create Graphics Pipeline for material");
        }

        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        if (vkCreateGraphicsPipelines(gil.Device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rp->linePipeline) != VK_SUCCESS)
        {
            Error("Failed to create Graphics Pipeline for material");
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = gil.DescriptorPool();
        allocInfo.descriptorSetCount = (u32)rp->dsSetLayouts.size();
        allocInfo.pSetLayouts = rp->dsSetLayouts.data();

        // need some temp storage for buffers & samplers
        int bufferInfoIdx = 0;
        int imageInfoIdx = 0;
        vector<VkDescriptorBufferInfo> bufferInfoCache(16);
        vector<VkDescriptorImageInfo> imageInfoCache(16);

        auto& sm = ShaderManager::Instance();
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            rp->descriptorSets[i].resize(allocInfo.descriptorSetCount);
            if (vkAllocateDescriptorSets(gil.Device(), &allocInfo, rp->descriptorSets[i].data()) != VK_SUCCESS)
                Error("failed to allocate descriptor sets!");

            vector<VkWriteDescriptorSet> descriptorWrites;
            for (auto& sro : shaderAD->SROs)
            {
                VkWriteDescriptorSet wds{};
                wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                wds.dstSet = rp->descriptorSets[i][rp->setMapping[sro.set]];
                wds.dstBinding = sro.binding;
                wds.dstArrayElement = 0;
                wds.descriptorCount = 1;

                switch (sro.type)
                {
                    case SROType_UBO:
                    {
                        VkDescriptorBufferInfo& bufferInfo = bufferInfoCache[bufferInfoIdx++];

                        UBOInfoInstance* uboInstance = nullptr;
                        auto mbo = FindMBO(mrpi, sro.name);
                        if (mbo)
                        {
                            uboInstance = mbo->uboInstance;
                        }
                        else
                        {
                            auto uboInfo = sm.FindUBO(sro.name);
                            uboInstance = uboInfo->dynamicInstance;
                        }

                        bufferInfo.buffer = uboInstance->platformData->buffer[i];
                        wds.descriptorType = uboInstance->isDynamic ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        bufferInfo.offset = 0;
                        bufferInfo.range = uboInstance->ubo->size;
                        wds.pBufferInfo = &bufferInfo;
                    }
                    break;

                    case SROType_Sampler:
                    {
                        VkDescriptorImageInfo& imageInfo = imageInfoCache[imageInfoIdx++];

                        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        auto sampler = FindSampler(mrpi, sro.varName);
                        Assert(sampler, STR("Cannot find texture '{}' in material '{}' referenced by the shader", sro.varName, assetData->name));
                        imageInfo.imageView = sampler->texture->GetPlatformData()->textureImageView;

                        VkFilter minFilter = (VkFilter)sampler->minFilter;
                        VkFilter maxFilter = (VkFilter)sampler->magFilter;
                        VkSamplerMipmapMode mipMapFilter = (VkSamplerMipmapMode)sampler->mipFilter;
                        VkSamplerAddressMode addressModeU = NeoAddressModeToVk[sampler->uWrap];
                        VkSamplerAddressMode addressModeV = NeoAddressModeToVk[sampler->vWrap];
                        VkCompareOp compareOp = NeoCompareOpToVk[sampler->compare];
                        gil.createTextureSampler(minFilter, maxFilter, mipMapFilter, addressModeU, addressModeV, compareOp, imageInfo.sampler);
                        wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        wds.pImageInfo = &imageInfo;
                    }
                    break;
                }

                descriptorWrites.push_back(wds);
            }
            vkUpdateDescriptorSets(gil.Device(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
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
        platformData->memOffset = -1;
        platformData->frameID = -1;
        platformData->data = new u8[uboInfo.size];
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

IADPlatformData* IADPlatformData_Create(InputAttributesDescription* iad)
{
    auto platformData = new IADPlatformData;

    for (auto& binding : iad->bindings)
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = binding.binding;
        bindingDescription.stride = binding.stride;
        bindingDescription.inputRate = binding.bindingIsPerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        platformData->bindingDescriptions.push_back(bindingDescription);
    }

    for (auto& attrib : iad->attributes)
    {
        VkVertexInputAttributeDescription attributeDescription{};
        attributeDescription.binding = attrib.binding;
        attributeDescription.location = attrib.location;
        attributeDescription.format = VertexFormatToVk(attrib.format);
        attributeDescription.offset = attrib.offset;
        platformData->attributeDescriptions.push_back(attributeDescription);
    }

    return platformData;
}


RenderPassPlatformData *RenderPassPlatformData_Create(RenderPassAssetData* assetData)
{
    auto &gil = GIL::Instance();
    auto platformData = new RenderPassPlatformData;

    // check if any color attachments are the swapchain
    for (auto& color : assetData->colorAttachments)
    {
        if (color.name == "swapchain")
        {
            platformData->useSwapChain = true;
        }
    }

    int framebufferCount = platformData->useSwapChain ? gil.GetSwapChainImageCount() : 1;
    vector<vector<VkImageView>> imageViewsList(framebufferCount);

    vector<VkAttachmentDescription> attachments;
    vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentReference depthAttachmentRef{};

    for (auto& color : assetData->colorAttachments)
    {
        bool useSwapChain = color.name == "swapchain";

        VkFormat fmt = useSwapChain ? gil.GetSwapChainImageFormat() : gil.FindVulkanFormat(color.fmt);
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = (u32)attachments.size();
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.emplace_back(colorAttachmentRef);

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = fmt;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = color.doClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.emplace_back(colorAttachment);

        VkClearValue clear{};
        clear.color = { color.clear.color.r, color.clear.color.g, color.clear.color.b, color.clear.color.a };
        platformData->clearValues.push_back(clear);

        for (int i = 0; i < framebufferCount; i++)
        {
            imageViewsList[i].push_back(useSwapChain ? gil.GetSwapChainImageView(i) : color.texture->GetPlatformData()->textureImageView);
        }
    }

    bool useDepth = !assetData->depthAttachment.name.empty();
    if (useDepth)
    {
        bool useSwapChain = assetData->depthAttachment.name == "swapchain";

        depthAttachmentRef.attachment = (u32)attachments.size();
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = useSwapChain ? gil.GetDepthFormat() : gil.FindVulkanFormat(assetData->depthAttachment.fmt);
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = assetData->depthAttachment.doClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.emplace_back(depthAttachment);

        for (int i = 0; i < framebufferCount; i++)
        {
            imageViewsList[i].push_back(useSwapChain ? gil.GetDepthBufferImageView() : assetData->depthAttachment.texture->GetPlatformData()->textureImageView);        }

        VkClearValue clear{};
        clear.depthStencil = { assetData->depthAttachment.clear.depth.depth, assetData->depthAttachment.clear.depth.stencil };
        platformData->clearValues.push_back(clear);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = (u32)colorAttachmentRefs.size();
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : nullptr;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (u32)attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;

    if (vkCreateRenderPass(gil.Device(), &renderPassInfo, nullptr, &platformData->renderPass) != VK_SUCCESS)
    {
        Error("failed to create render pass!");
    }

    platformData->frameBuffers.resize(framebufferCount);

    ivec2 size = (platformData->useSwapChain) ? gil.GetSwapChainImageSize() : assetData->size;
    for (int i = 0; i < framebufferCount; i++)
    {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = platformData->renderPass;
        framebufferInfo.attachmentCount = (u32)imageViewsList[i].size();
        framebufferInfo.pAttachments = imageViewsList[i].data();
        framebufferInfo.width = size.x;
        framebufferInfo.height = size.y;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(gil.Device(), &framebufferInfo, nullptr, &platformData->frameBuffers[i]) != VK_SUCCESS)
        {
            Error("failed to create framebuffer!");
        }
    }

    return platformData;
}

