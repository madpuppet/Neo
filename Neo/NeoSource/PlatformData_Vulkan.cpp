#include "Neo.h"
#include "PlatformData_Vulkan.h"
#include "Texture.h"
#include "Shader.h"
#include "Material.h"
#include "Model.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

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
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};



TexturePlatformData* TexturePlatformData_Create(TextureAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    Log(STR("PLATFORM DATA for TEXTURE: {}", assetData->name));

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

ShaderPlatformData* ShaderPlatformData_Create(struct ShaderAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    Log(STR("PLATFORM DATA for SHADER: {}", assetData->name));

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

MaterialPlatformData* MaterialPlatformData_Create(MaterialAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    Log(STR("PLATFORM DATA for MATERIAL: {}", assetData->name));

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
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
        VK_DYNAMIC_STATE_SCISSOR
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

ModelPlatformData* ModelPlatformData_Create(struct ModelAssetData* assetData)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    Log(STR("PLATFORM DATA for MODEL: {}", assetData->name));

    auto& gil = GIL::Instance();
    auto platformData = new ModelPlatformData;
    auto device = gil.Device();

    // vertex buffer
    {
        VkDeviceSize bufferSize = sizeof(assetData->verts[0]) * assetData->verts.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        gil.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, assetData->verts.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        gil.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, platformData->vertexBuffer, platformData->vertexBufferMemory);
        gil.copyBuffer(stagingBuffer, platformData->vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    // index buffer
    {
        VkDeviceSize bufferSize = sizeof(assetData->indices[0]) * assetData->indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        gil.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, assetData->indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        gil.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, platformData->indexBuffer, platformData->indexBufferMemory);

        gil.copyBuffer(stagingBuffer, platformData->indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
        platformData->indiceCount = (int)assetData->indices.size();
    }

    return platformData;
}

void ModelPlatformData_Destroy(ModelPlatformData* platformData)
{
}





