#include "Neo.h"
#include "GIL_Vulkan.h"
#include "Texture.h"
#include "Material.h"
#include "StaticMesh.h"
#include "ShaderManager.h"
#include "RenderPass.h"
#include "View.h"

DECLARE_MODULE(GIL, NeoModuleInitPri_GIL, NeoModulePri_None);

#include <tiny_obj_loader.h>

const vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const vector<const char*> deviceExtensions = {
    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static std::vector<Vertex_p3f_t2f_c4b> s_vertices;
static std::vector<uint32_t> s_indices;

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    LOG(Gfx, STR("Warning - validation layer: {}", pCallbackData->pMessage));
    return VK_FALSE;
}

void GIL::StartupMainThread()
{
    // create WINDOW
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
    {
        LOG(Gfx, STR("SDL could not initialize! SDL Error: {}\n", SDL_GetError()));
        exit(0);
    }

    m_window = SDL_CreateWindow(APP_TITLE "v" VERSION, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_swapChainImageSize.x, m_swapChainImageSize.y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_VULKAN);
    m_joystick = SDL_JoystickOpen(0);
}

static VkFormat s_VertexFormatToVk[] =
{
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_R32G32_SFLOAT,
    VK_FORMAT_R32G32B32_SFLOAT
};
VkFormat VertexFormatToVk(VertexFormat vf) { return s_VertexFormatToVk[vf]; }
static string s_VertexFormatToString[] = { "vec4", "vec2", "vec3" };
string VertexFormatToString(VertexFormat vf) { return s_VertexFormatToString[vf]; }

void GIL::Initialize()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createFormatMappings();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createUniformBufferDynamicMemory();
    createDescriptorPool();
    createCommandBuffers();
    createSyncObjects();
#if PROFILING_ENABLED
    createTimeQueries();
#endif
}

void GIL::createFormatMappings()
{
    m_neoFormatToVulkanFormat[PixFmt_Undefined] = VK_FORMAT_UNDEFINED;

    m_neoFormatToVulkanFormat[PixFmt_R4G4_UNORM] = VK_FORMAT_R4G4_UNORM_PACK8;
    m_neoFormatToVulkanFormat[PixFmt_R4G4B4A4_UNORM] = VK_FORMAT_R4G4B4A4_UNORM_PACK16;
    m_neoFormatToVulkanFormat[PixFmt_R5G6B5_UNORM] = VK_FORMAT_R5G6B5_UNORM_PACK16;
    m_neoFormatToVulkanFormat[PixFmt_R5G6B5A1_UNORM] = VK_FORMAT_R5G5B5A1_UNORM_PACK16;

    m_neoFormatToVulkanFormat[PixFmt_R8_UNORM] = VK_FORMAT_R8_UNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8_SNORM] = VK_FORMAT_R8_SNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8_UINT] = VK_FORMAT_R8_UINT;
    m_neoFormatToVulkanFormat[PixFmt_R8_SINT] = VK_FORMAT_R8_SINT;
    m_neoFormatToVulkanFormat[PixFmt_R8_SRGB] = VK_FORMAT_R8_SRGB;

    m_neoFormatToVulkanFormat[PixFmt_R8G8_UNORM] = VK_FORMAT_R8G8_UNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8G8_SNORM] = VK_FORMAT_R8G8_SNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8G8_UINT] = VK_FORMAT_R8G8_UINT;
    m_neoFormatToVulkanFormat[PixFmt_R8G8_SINT] = VK_FORMAT_R8G8_SINT;

    m_neoFormatToVulkanFormat[PixFmt_R8G8B8A8_UNORM] = VK_FORMAT_R8G8B8A8_UNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8G8B8A8_SNORM] = VK_FORMAT_R8G8B8A8_SNORM;
    m_neoFormatToVulkanFormat[PixFmt_R8G8B8A8_UINT] = VK_FORMAT_R8G8B8A8_UINT;
    m_neoFormatToVulkanFormat[PixFmt_R8G8B8A8_SINT] = VK_FORMAT_R8G8B8A8_SINT;
    m_neoFormatToVulkanFormat[PixFmt_R8G8B8A8_SRGB] = VK_FORMAT_R8G8B8A8_SRGB;

    m_neoFormatToVulkanFormat[PixFmt_B8G8R8A8_UNORM] = VK_FORMAT_B8G8R8A8_UNORM;
    m_neoFormatToVulkanFormat[PixFmt_B8G8R8A8_SNORM] = VK_FORMAT_B8G8R8A8_SNORM;
    m_neoFormatToVulkanFormat[PixFmt_B8G8R8A8_UINT] = VK_FORMAT_B8G8R8A8_UINT;
    m_neoFormatToVulkanFormat[PixFmt_B8G8R8A8_SINT] = VK_FORMAT_B8G8R8A8_SINT;
    m_neoFormatToVulkanFormat[PixFmt_B8G8R8A8_SRGB] = VK_FORMAT_B8G8R8A8_SRGB;

    m_neoFormatToVulkanFormat[PixFmt_R16_UNORM] = VK_FORMAT_R16_UNORM;
    m_neoFormatToVulkanFormat[PixFmt_R16_SNORM] = VK_FORMAT_R16_SNORM;
    m_neoFormatToVulkanFormat[PixFmt_R16_UINT] = VK_FORMAT_R16_UINT;
    m_neoFormatToVulkanFormat[PixFmt_R16_SINT] = VK_FORMAT_R16_SINT;
    m_neoFormatToVulkanFormat[PixFmt_R16_SFLOAT] = VK_FORMAT_R16_SFLOAT;

    m_neoFormatToVulkanFormat[PixFmt_B10G11R11_UFLOAT] = VK_FORMAT_B10G11R11_UFLOAT_PACK32;

    m_neoFormatToVulkanFormat[PixFmt_BC1_RGB_UNORM] = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    m_neoFormatToVulkanFormat[PixFmt_BC1_RGB_SRGB] = VK_FORMAT_BC1_RGB_SRGB_BLOCK;
    m_neoFormatToVulkanFormat[PixFmt_BC3_RGBA_UNORM] = VK_FORMAT_BC3_UNORM_BLOCK;
    m_neoFormatToVulkanFormat[PixFmt_BC3_RGBA_SRGB] = VK_FORMAT_BC3_SRGB_BLOCK;

    m_neoFormatToVulkanFormat[PixFmt_D32_SFLOAT] = VK_FORMAT_D32_SFLOAT;
    m_neoFormatToVulkanFormat[PixFmt_D24_UNORM_S8_UINT] = VK_FORMAT_D24_UNORM_S8_UINT;

    // validate the hardware supports all these texture types
    for (auto fmt : m_neoFormatToVulkanFormat)
    {
        if (fmt.first != PixFmt_Undefined)
        {
            // Check supported features for the format
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, fmt.second, &formatProperties);

            if (!(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                LOG(Texture, STR("Texture Format {} => {} Not Supported", (int)fmt.first, (int)fmt.second));
        }
    }
}

void GIL::Shutdown()
{
}

void GIL::FrameWait()
{
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &m_frameSwapImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_frameBufferInvalid = true;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        Error("failed to acquire swap chain image!");
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
}

void GIL::BeginFrame()
{
    auto &commandBuffer = m_commandBuffers[m_currentFrame];

    // just increment forever. we can mark resources with this to tell if they've been accessed this frame already.
    // the game would have to run for 250 days at 200fps to wrap this.
    m_uniqueFrameidx++;

    // start command buffer
    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        Error("failed to begin recording command buffer!");
    }

#if PROFILING_ENABLED
    vkCmdResetQueryPool(commandBuffer, m_queryPool, 0, MaxProfileTimestamps);
#endif

    m_swapChainColorLayout = TextureLayout_Undefined;
    m_swapChainDepthLayout = TextureLayout_Undefined;

    m_boundMaterial = nullptr;
    m_boundRenderPass = nullptr;
    m_boundView = nullptr;


    TransitionSwapChainColorImage(TextureLayout_ColorAttachment);
}

mat4x4 InverseSimple(const mat4x4 & m)
{
    mat4x4 temp;
    temp[0] = { m[0][0], m[1][0], m[2][0], 0 };
    temp[1] = { m[0][1], m[1][1], m[2][1], 0 };
    temp[2] = { m[0][2], m[1][2], m[2][2], 0 };

    temp[3][0] = -(m[3][0] * m[0][0] + m[3][1] * m[0][1] + m[3][2] * m[0][2]);
    temp[3][1] = -(m[3][0] * m[1][0] + m[3][1] * m[1][1] + m[3][2] * m[1][2]);
    temp[3][2] = -(m[3][0] * m[2][0] + m[3][1] * m[2][1] + m[3][2] * m[2][2]);
    temp[3][3] = 1.0f;
    return temp;
}

void GIL::UpdateUBOInstanceMember(UBOInfoInstance* uboInstance, u32 memberOffset, const void *data, u32 datasize, bool flush)
{
    auto pd = uboInstance->platformData;
    if (uboInstance->isDynamic)
    {
        memcpy((u8*)pd->data + memberOffset, data, datasize);
        if (flush)
            UpdateUBOInstance(uboInstance, uboInstance->platformData->data, uboInstance->ubo->size, true);
    }
    else
    {
        memcpy((u8*)pd->memoryMapped[m_currentFrame] + memberOffset, data, datasize);
    }
}

void GIL::UpdateUBOInstance(UBOInfoInstance *uboInstance, void* uboMem, u32 uboSize, bool updateBoundMaterial)
{
    Assert(Thread::GetCurrentThreadGUID() == ThreadGUID_Render, "This needs to run on render thread because it access the active render pass");

    auto uboPD = uboInstance->platformData;
    if (uboInstance->isDynamic)
    {
        int size = (uboSize + 63) & ~63;
        uboPD->memOffset = m_dynamicUniformBufferMemoryUsed;
        memcpy((u8*)uboPD->data, uboMem, uboSize);
        memcpy((u8*)m_dynamicUniformBufferMemoryMapped[m_currentFrame] + m_dynamicUniformBufferMemoryUsed, uboMem, uboSize);
        m_dynamicUniformBufferMemoryUsed += size;

        // update bound material if its applicable to the current render pass
        if (m_boundMaterial && updateBoundMaterial)
        {
            auto materialAD = m_boundMaterial->GetAssetData();
            auto materialPD = m_boundMaterial->GetPlatformData();

            MaterialRenderPassInfo* mrpi = nullptr;
            MaterialPlatformRenderPassData* mprp = nullptr;
            for (int i=0; i<materialAD->renderPasses.size(); i++)
            {
                if (materialAD->renderPasses[i]->renderPass == m_boundRenderPass)
                {
                    mrpi = materialAD->renderPasses[i];
                    mprp = materialPD->renderPasses[i];
                    break;
                }
            }

            if (mrpi && mprp)
            {
                auto shaderAD = mrpi->shader->GetAssetData();
                auto shaderPD = mrpi->shader->GetPlatformData();
                auto commandBuffer = m_commandBuffers[m_currentFrame];

                // check if bound material references this UBO
                // we need to update any sets after this UBO
                u32 dynamicOffsets[16]; // maximum of 16 should be enough for any shader
                int idx = 0;
                int set = -1;
                bool started = false;
                int startedSet = -1;
                for (auto& it : mprp->uboInstances)
                {
                    if (it.first != set && !started)
                    {
                        idx = 0;
                        set = it.first;
                    }

                    if (it.second == uboInstance)
                    {
                        started = true;
                        startedSet = set;
                    }

                    //if this ubo hasn't had its dymamic 
                    if (it.second->isDynamic)
                    {
                        if (it.second->platformData->memOffset == (u32)-1)
                        {
                            Assert(it.second != uboInstance, "internal logic error - memoffset should have been set above");
                            UpdateUBOInstance(it.second, it.second->platformData->data, it.second->ubo->size, false);
                            Assert(it.second->platformData->memOffset != (u32)-1, "update instance didn't work!");
                        }
                        dynamicOffsets[idx++] = it.second->platformData->memOffset;
                    }
                }

                if (startedSet != -1)
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mprp->pipelineLayout, startedSet,
                        (u32)mprp->descriptorSets[m_currentFrame].size() - startedSet, &mprp->descriptorSets[m_currentFrame][startedSet],
                        idx, dynamicOffsets);
                }
            }
        }
    }
    else // just a static instance - it only updates once per frame and always points to the same memory, so no need to rebind materials, etc
    {
        memcpy((u8*)uboPD->memoryMapped[m_currentFrame], uboMem, uboSize);
        Assert(uboPD->frameID != m_uniqueFrameidx, "Should not update a static UBO twice in one frame!");
    }
    uboPD->frameID = m_uniqueFrameidx;
}

void GIL::SetViewport(const rect& viewport, float minDepth, float maxDepth)
{
#if 0
    VkViewport vp{};
    vp.x = viewport.x * m_swapChainImageSize.x;
    vp.y = viewport.y * m_swapChainImageSize.y;
    vp.width = viewport.w * m_swapChainImageSize.x;
    vp.height = viewport.h * m_swapChainImageSize.y;
    vp.minDepth = minDepth;
    vp.maxDepth = maxDepth;
    vkCmdSetViewport(m_commandBuffers[m_currentFrame], 0, 1, &vp);
#endif
}

void GIL::SetScissor(const rect& scissorRect)
{
#if 0
    VkRect2D scissor{};
    scissor.offset.x = (u32)(scissorRect.x * m_swapChainImageSize.x);
    scissor.offset.y = (u32)(scissorRect.y * m_swapChainImageSize.y);
    scissor.extent.width = (u32)(scissorRect.w * m_swapChainImageSize.x);
    scissor.extent.height = (u32)(scissorRect.h * m_swapChainImageSize.y);
    vkCmdSetScissor(m_commandBuffers[m_currentFrame], 0, 1, &scissor);
#endif
}

void GIL::EndFrame()
{
    auto commandBuffer = m_commandBuffers[m_currentFrame];

    if (m_boundRenderPass)
        SetRenderPass(nullptr);
    
    // transition swapchain image to present layout
    TransitionSwapChainColorImage(TextureLayout_Present);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        Error("failed to record command buffer!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (!m_frameBufferInvalid)
    {
        auto rc = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
        if (rc != VK_SUCCESS)
        {
            Error(STR("failed to submit draw command buffer! rc {}", (int)rc));
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { m_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &m_frameSwapImage;

        VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            m_frameBufferInvalid = true;
        }
        else if (result != VK_SUCCESS)
        {
            Error("Failed to present swap chain image!");
        }
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    m_dynamicUniformBufferMemoryUsed = 0;
}

void GIL::createSurface()
{
    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);
}

void GIL::pickPhysicalDevice() 
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) 
    {
        Error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (isDeviceSuitable(device)) 
        {
            m_physicalDevice = device;
            break;
        }
    }

    if (m_physicalDevice == VK_NULL_HANDLE)
    {
        Error("failed to find a suitable GPU!");
    }
}

bool GIL::isDeviceSuitable(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

    return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

void GIL::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (m_enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else 
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
    {
        Error("failed to create logical device!");
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

GIL::QueueFamilyIndices GIL::findQueueFamilies(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

void GIL::createInstance() {
    if (m_enableValidationLayers && !checkValidationLayerSupport())
    {
        Error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    const void** nextCreate = &createInfo.pNext;
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        nextCreate = &debugCreateInfo.pNext;
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

#if 0
#if PROFILING_ENABLED
    // Enable the hostQueryReset feature
    VkPhysicalDeviceFeatures2 features2 = {};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures = {};
    hostQueryResetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT;
    hostQueryResetFeatures.hostQueryReset = VK_TRUE;
    features2.pNext = &hostQueryResetFeatures;
    *nextCreate = &features2;
#endif
#endif

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) 
    {
        Error("failed to create instance!");
    }
}

bool GIL::checkValidationLayerSupport() 
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

vector<const char*> GIL::getRequiredExtensions()
{
    const char* sdl_extensions[256];
    uint32_t sdl_extensionCount = 256;

    SDL_Vulkan_GetInstanceExtensions(m_window, &sdl_extensionCount, sdl_extensions);

    std::vector<const char*> extensions;
    for (uint32_t i = 0; i < sdl_extensionCount; i++)
        extensions.push_back(sdl_extensions[i]);

    if (m_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    return extensions;
}

void GIL::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

void GIL::setupDebugMessenger() 
{
    if (!m_enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) 
    {
        Error("failed to set up debug messenger!");
    }
}

bool GIL::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

GIL::SwapChainSupportDetails GIL::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

    if (formatCount != 0) 
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) 
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void GIL::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else 
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
    {
        Error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;
    m_depthFormat = findDepthFormat();
}

VkSurfaceFormatKHR GIL::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) 
{
    for (const auto& availableFormat : availableFormats) 
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR GIL::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
//#if !PROFILING_ENABLED
    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
//#endif
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D GIL::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { 1280, 720 };
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}

void GIL::createImageViews()
{
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
    {
        m_swapChainImageViews[i] = createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

VkImageView GIL::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) 
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
    {
        Error("failed to create image view!");
    }

    return imageView;
}

void GIL::createRenderPass() 
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = m_depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

//    VkSubpassDependency dependency{};
//    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//    dependency.dstSubpass = 0;
//    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
//    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<u32>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = nullptr;// &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) 
    {
        Error("failed to create render pass!");
    }
}

VkFormat GIL::findDepthFormat() 
{
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat GIL::findSupportedFormat(const vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    Error("failed to find supported format!");
    return VK_FORMAT_UNDEFINED;
}


void GIL::createFramebuffers() 
{
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            m_swapChainImageViews[i],
            m_depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            Error("failed to create framebuffer!");
        }
    }
}

void GIL::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
    {
        Error("failed to create graphics command pool!");
    }
}

VkShaderModule GIL::createShaderModule(const std::vector<char>& code) 
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
    {
        Error("failed to create shader module!");
    }

    return shaderModule;
}

void GIL::createDepthResources()
{
    VkFormat depthFormat = findDepthFormat();

    createImage(m_swapChainExtent.width, m_swapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
    m_depthImageView = createImageView(m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void GIL::createImage(u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        Error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        Error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_device, image, imageMemory, 0);
}

uint32_t GIL::findMemoryType(u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    Error("failed to find suitable memory type!");
    return 0;
}

bool GIL::createTextureSampler(VkFilter minFilter, VkFilter maxFilter, VkSamplerMipmapMode mipMapFilter, VkSamplerAddressMode addressingU, VkSamplerAddressMode addressingV, VkCompareOp compareOp, VkSampler &sampler)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = maxFilter;
    samplerInfo.minFilter = minFilter;
    samplerInfo.addressModeU = addressingU;
    samplerInfo.addressModeV = addressingV;
    samplerInfo.addressModeW = addressingU;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = (compareOp == VK_COMPARE_OP_ALWAYS) ? VK_FALSE : VK_TRUE;
    samplerInfo.compareOp = compareOp;
    samplerInfo.mipmapMode = mipMapFilter;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
    {
        Error("failed to create texture sampler!");
        return false;
    }

    return true;
}

void GIL::copyMemoryToBuffer(VkDeviceMemory bufferMemory, void *memory, size_t size)
{
    void* mappedVertexMemory;
    vkMapMemory(m_device, bufferMemory, 0, size, 0, &mappedVertexMemory);
    memcpy(mappedVertexMemory, memory, size);

    VkMappedMemoryRange vertexMemoryRange = {};
    vertexMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    vertexMemoryRange.memory = bufferMemory;
    vertexMemoryRange.offset = 0;
    vertexMemoryRange.size = size;
    vkFlushMappedMemoryRanges(m_device, 1, &vertexMemoryRange);

    vkUnmapMemory(m_device, bufferMemory);
}

void GIL::createUniformBuffer(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT>& memory,
    array<void*, MAX_FRAMES_IN_FLIGHT>& mapped, u32 size)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, buffer[i], memory[i]);
        vkMapMemory(m_device, memory[i], 0, size, 0, &mapped[i]);
    }
}
void GIL::createUniformBufferDynamic(array<VkBuffer, MAX_FRAMES_IN_FLIGHT>& buffer, array<void*, MAX_FRAMES_IN_FLIGHT>& mapped)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = MaxDynamicUniformBufferMemory;
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer[i]) != VK_SUCCESS)
            Error("failed to create buffer!");
        mapped[i] = m_dynamicUniformBufferMemoryMapped[i];
        vkBindBufferMemory(m_device, buffer[i], m_dynamicUniformBufferMemory[i], 0);
    }
}

u32 GIL::AllocateDynamicUniformBufferMemory(u32 size)
{
    u32 retval = m_dynamicUniformBufferMemoryUsed;
    m_dynamicUniformBufferMemoryUsed += ((size + 63) & ~63);
    Assert(m_dynamicUniformBufferMemoryUsed < MaxDynamicUniformBufferMemory, "Out of uniform buffer dynamic memory!");
    return retval;
}

void GIL::createUniformBufferDynamicMemory()
{
    VkDeviceSize size = (VkDeviceSize)MaxDynamicUniformBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer tempBuffer;
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &tempBuffer) != VK_SUCCESS)
        Error("failed to create buffer!");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, tempBuffer, &memRequirements);
    vkDestroyBuffer(m_device, tempBuffer, nullptr);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        auto rc = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_dynamicUniformBufferMemory[i]);
        if (rc != VK_SUCCESS)
        {
            Error(STR("failed to allocate buffer memory! RC {}", (int)rc));
        }
        vkMapMemory(m_device, m_dynamicUniformBufferMemory[i], 0, size, 0, &m_dynamicUniformBufferMemoryMapped[i]);
    }

}




void GIL::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
    {
        Error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    auto rc = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);
    if (rc != VK_SUCCESS)
    {
        Error(STR("failed to allocate buffer memory! RC {}", (int)rc));
    }

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void GIL::createDynamicBuffer(VkDeviceSize size, VkDeviceSize memorySize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = memorySize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        Error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memorySize;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    auto rc = vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory);
    if (rc != VK_SUCCESS)
    {
        Error(STR("failed to allocate buffer memory! RC {}", (int)rc));
    }

    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}


void GIL::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

VkCommandBuffer GIL::beginSingleTimeCommands() 
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void GIL::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void GIL::createDescriptorPool() 
{
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT*30);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT*30);
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT*30);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT*90);
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) 
    {
        Error("failed to create descriptor pool!");
    }
}

void GIL::createCommandBuffers() 
{
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) 
    {
        Error("failed to allocate command buffers!");
    }
}

void GIL::createSyncObjects()
{
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) 
        {
            Error("failed to create synchronization objects for a frame!");
        }
    }
}

void GIL::WaitForGPU()
{
    vkDeviceWaitIdle(m_device);
}

void GIL::recreateSwapChain() 
{
    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
    m_frameBufferInvalid = false;
}

void GIL::cleanupSwapChain() 
{
    for (auto framebuffer : m_swapChainFramebuffers)
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    m_swapChainFramebuffers.clear();

    vkDestroyImageView(m_device, m_depthImageView, nullptr);
    m_depthImageView = nullptr;

    vkDestroyImage(m_device, m_depthImage, nullptr);
    m_depthImage = nullptr;

    vkFreeMemory(m_device, m_depthImageMemory, nullptr);
    m_depthImageMemory = nullptr;

    for (auto imageView : m_swapChainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);
    m_swapChainImageViews.clear();

    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    m_swapChain = nullptr;
}

VkImageLayout s_textureLayoutToVkImageLayout[] =
{
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
};

VkAccessFlags s_textureLayoutToVkAccessFlags[] =
{
    VK_ACCESS_NONE,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_SHADER_READ_BIT,
    VK_ACCESS_MEMORY_READ_BIT
};

VkPipelineStageFlagBits s_textureLayoutToVkPipelineStageFlags[] =
{
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
};

void GIL::TransitionSwapChainColorImage(TextureLayout newLayout)
{
    TransitionImageLayout(m_swapChainImages[m_frameSwapImage], false, m_swapChainColorLayout, newLayout);
    m_swapChainColorLayout = newLayout;
}

void GIL::TransitionSwapChainDepthImage(TextureLayout newLayout)
{
    TransitionImageLayout(m_depthImage, true, m_swapChainDepthLayout, newLayout);
    m_swapChainDepthLayout = newLayout;
}

void GIL::TransitionTexture(Texture* texture, TextureLayout currentLayout, TextureLayout newLayout)
{
    auto textureAD = texture->GetAssetData();
    auto texturePD = texture->GetPlatformData();
    bool isDepth = (textureAD->format == PixFmt_D24_UNORM_S8_UINT) || (textureAD->format == PixFmt_D32_SFLOAT);
    TransitionImageLayout(texturePD->textureImage, isDepth, currentLayout, newLayout);
}

void GIL::TransitionImageLayout(VkImage image, bool isDepth, TextureLayout currentLayout, TextureLayout newLayout)
{
    auto commandBuffer = m_commandBuffers[m_currentFrame];
    VkImageLayout oldIL = s_textureLayoutToVkImageLayout[currentLayout];
    VkImageLayout newIL = s_textureLayoutToVkImageLayout[newLayout];

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldIL;
    barrier.newLayout = newIL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = s_textureLayoutToVkAccessFlags[currentLayout];
    barrier.dstAccessMask = s_textureLayoutToVkAccessFlags[newLayout];
    VkPipelineStageFlags sourceStage = s_textureLayoutToVkPipelineStageFlags[currentLayout];
    VkPipelineStageFlags destinationStage = s_textureLayoutToVkPipelineStageFlags[newLayout];
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void GIL::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        Error("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

void GIL::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);
}

void GIL::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);

    Assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,"texture image format does not support linear blitting!");

    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(commandBuffer);
}

// use material
void GIL::BindMaterial(Material* material, bool lines)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    auto materialPD = material->GetPlatformData();
    auto materialAD = material->GetAssetData();
    if (materialPD)
    {
        MaterialPlatformRenderPassData *mprpd = nullptr;
        MaterialRenderPassInfo* mrpi = nullptr;
        for (int i = 0; i < materialAD->renderPasses.size(); i++)
        {
            if (materialAD->renderPasses[i]->renderPass == m_boundRenderPass)
            {
                mrpi = materialAD->renderPasses[i];
                mprpd = materialPD->renderPasses[i];
                break;
            }
        }

        auto commandBuffer = m_commandBuffers[m_currentFrame];
        if (lines)
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mprpd->linePipeline);
        else
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mprpd->polygonPipeline);

        u32 dynamicOffsets[16]; // maximum of 16 should be enough for any shader
        int dynamicOffsetCount = 0;
        auto shader = mrpi->shader;
        auto shaderPD = shader->GetPlatformData();
        auto shaderAD = shader->GetAssetData();
        for (auto &it : mprpd->uboInstances)
        {
            auto uboInstance = it.second;
            if (uboInstance->isDynamic)
            {
                if (uboInstance->platformData->frameID != m_uniqueFrameidx)
                {
                    UpdateUBOInstance(uboInstance, uboInstance->platformData->data, uboInstance->ubo->size, false);
                }
                Assert(uboInstance->platformData->memOffset != (u32)-1, STR("Error of UBO {} before call to UpdateDynamicUBO", uboInstance->ubo->structName));
                dynamicOffsets[dynamicOffsetCount++] = uboInstance->platformData->memOffset;
            }
        }

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mprpd->pipelineLayout, 0,
            (u32)mprpd->descriptorSets[m_currentFrame].size(), mprpd->descriptorSets[m_currentFrame].data(),
            dynamicOffsetCount, dynamicOffsets);
        m_boundMaterial = material;
    }
}

// bind geometry buffers
void GIL::BindGeometryBuffer(NeoGeometryBuffer* buffer)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    auto commandBuffer = m_commandBuffers[m_currentFrame];
    VkBuffer vertexBuffers[] = { buffer->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    if (buffer->indexBuffer)
    {
        vkCmdBindIndexBuffer(commandBuffer, buffer->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

void GIL::SetRenderPrimitiveType(PrimType primType)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));
    auto commandBuffer = m_commandBuffers[m_currentFrame];
    vkCmdSetPrimitiveTopology(commandBuffer, VkPrimitiveTopology(primType));
}


// render primitive
void GIL::RenderPrimitive(u32 vertStart, u32 vertCount, u32 indexStart, u32 indexCount)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    auto commandBuffer = m_commandBuffers[m_currentFrame];
    if (indexCount == 0)
    {
        vkCmdDraw(commandBuffer, vertCount, 1, vertStart, 0);
    }
    else
    {
        vkCmdDrawIndexed(commandBuffer, indexCount, 1, indexStart, 0, 0);
    }
}


void GIL::RenderStaticMesh(StaticMesh *mesh)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    // check platform data has been created successfully
    auto meshPD = mesh->GetPlatformData();
    auto meshAD = mesh->GetAssetData();
    auto material = *(meshAD->material);
    if (!meshPD || !meshAD || !material || !meshPD->geomBuffer)
        return;

    BindMaterial(material, false);
    BindGeometryBuffer(meshPD->geomBuffer);
    SetRenderPrimitiveType(PrimType_TriangleList);
    RenderPrimitive(0, 0, 0, meshPD->indiceCount);
}

void GIL::RenderStaticMeshInstances(class StaticMesh* mesh, mat4x4* ltw, u32 ltwCount)
{
    Assert(Thread::IsOnThread(ThreadGUID_Render), STR("{} must be run on render thread,  currently on thread {}", __FUNCTION__, Thread::GetCurrentThreadGUID()));

    // check platform data has been created successfully
    auto meshPD = mesh->GetPlatformData();
    auto meshAD = mesh->GetAssetData();
    auto material = *(meshAD->material);
    auto uboInstance = ShaderManager::Instance().FindUBO("UBO_Model")->dynamicInstance;
    auto& gil = GIL::Instance();
    if (!meshPD || !meshAD || !material || !meshPD->geomBuffer || !uboInstance)
        return;

    BindMaterial(material, false);
    BindGeometryBuffer(meshPD->geomBuffer);
    SetRenderPrimitiveType(PrimType_TriangleList);
    for (u32 i = 0; i < ltwCount; i++)
    {
        gil.UpdateUBOInstance(uboInstance, &ltw[i], sizeof(mat4x4), true);
        RenderPrimitive(0, 0, 0, meshPD->indiceCount);
    }
}


bool GIL::ShowMessageBox(const string& string)
{
    SDL_MessageBoxButtonData buttons[2];
    buttons[0].flags = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    buttons[0].buttonid = 1;
    buttons[0].text = "Break";
    buttons[1].flags = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
    buttons[1].buttonid = 0;
    buttons[1].text = "Ignore";

    SDL_MessageBoxData data;
    data.flags = SDL_MESSAGEBOX_ERROR;
    data.window = m_window;
    data.title = "Neo Assert Hit...";
    data.message = string.c_str();
    data.numbuttons = 2;
    data.buttons = buttons;
    data.colorScheme = nullptr;
    int result = 0;
    SDL_ShowMessageBox(&data, &result);
    return result == 1;
}

float GIL::GetJoystickAxis(int idx)
{
    float joyval = Clamp(SDL_JoystickGetAxis(m_joystick, idx) / 32767.0f, -1.0f, 1.0f);
    if (abs(joyval) < 0.1f)
        joyval = 0.0f;
    return joyval;
}

NeoGeometryBuffer* GIL::CreateGeometryBuffer(void* vertData, u32 vertDataSize, void* indexData, u32 indexDataSize)
{
    auto buffer = new NeoGeometryBuffer;

    createBuffer(vertDataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer->vertexBuffer, buffer->vertexBufferMemory);
    buffer->vertexBufferSize = vertDataSize;

    if (vertData)
        copyMemoryToBuffer(buffer->vertexBufferMemory, vertData, vertDataSize);

    if (indexDataSize)
    {
        createBuffer(indexDataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            buffer->indexBuffer, buffer->indexBufferMemory);

        if (indexData)
            copyMemoryToBuffer(buffer->indexBufferMemory, indexData, indexDataSize);
    }
    buffer->indexBufferSize = indexDataSize;

    return buffer;
}
void GIL::DestroyGeometryBuffer(NeoGeometryBuffer* buffer)
{
    if (buffer)
    {
        vkDestroyBuffer(m_device, buffer->vertexBuffer, nullptr);
        vkFreeMemory(m_device, buffer->vertexBufferMemory, nullptr);
        if (buffer->indexBuffer)
        {
            vkDestroyBuffer(m_device, buffer->indexBuffer, nullptr);
            vkFreeMemory(m_device, buffer->indexBufferMemory, nullptr);
        }
        delete buffer;
    }
}
void GIL::MapGeometryBufferMemory(NeoGeometryBuffer* buffer, void**vertexMem, void**indexMem)
{
    vkMapMemory(m_device, buffer->vertexBufferMemory, 0, buffer->vertexBufferSize, 0, vertexMem);
    if (buffer->indexBufferSize)
        vkMapMemory(m_device, buffer->indexBufferMemory, 0, buffer->indexBufferSize, 0, indexMem);
}
void GIL::FlushGeometryBufferMemory(NeoGeometryBuffer* buffer, u32 vertDataSize, u32 indexDataSize)
{
    vertDataSize = (vertDataSize + 63) & ~63;
    indexDataSize = (indexDataSize + 63) & ~63;

    int flushCount = 1;
    VkMappedMemoryRange vertexMemoryRange[2] = {};
    vertexMemoryRange[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    vertexMemoryRange[0].memory = buffer->vertexBufferMemory;
    vertexMemoryRange[0].offset = 0;
    vertexMemoryRange[0].size = vertDataSize;

    if (buffer->indexBufferSize && indexDataSize)
    {
        flushCount = 2;
        vertexMemoryRange[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        vertexMemoryRange[1].memory = buffer->indexBufferMemory;
        vertexMemoryRange[1].offset = 0;
        vertexMemoryRange[1].size = indexDataSize;
    }
    vkFlushMappedMemoryRanges(m_device, flushCount, vertexMemoryRange);
}
void GIL::UnmapGeometryBufferMemory(NeoGeometryBuffer* buffer)
{
    vkUnmapMemory(m_device, buffer->vertexBufferMemory);

    if (buffer->indexBufferSize)
        vkUnmapMemory(m_device, buffer->indexBufferMemory);
}

void GIL::SetRenderPass(RenderPass* renderPass)
{
    if (m_boundRenderPass == renderPass)
        return;

    auto& commandBuffer = m_commandBuffers[m_currentFrame];

    // transition the old renderpass textures back to being shader textures
    if (m_boundRenderPass)
    {
        vkCmdEndRenderPass(commandBuffer);
        auto oldAD = m_boundRenderPass->GetAssetData();
        for (auto& col : oldAD->colorAttachments)
        {
            if (col.useSwapChain)
                TransitionSwapChainColorImage(TextureLayout_ShaderRead);
            else
                col.texture->SetLayout(TextureLayout_ShaderRead);
        }
        if (oldAD->depthAttachment.useSwapChain)
            TransitionSwapChainDepthImage(TextureLayout_ShaderRead);
        else if (oldAD->depthAttachment.texture != nullptr)
            oldAD->depthAttachment.texture->SetLayout(TextureLayout_ShaderRead);
    }

    m_boundRenderPass = renderPass;

    if (renderPass)
    {
        auto renderPassPD = renderPass->GetPlatformData();
        auto renderPassAD = renderPass->GetAssetData();

        // transition render pass textures to attachment layout
        for (auto& col : renderPassAD->colorAttachments)
        {
            if (col.useSwapChain)
                TransitionSwapChainColorImage(TextureLayout_ColorAttachment);
            else
                col.texture->SetLayout(TextureLayout_ColorAttachment);
        }
        if (renderPassAD->depthAttachment.useSwapChain)
            TransitionSwapChainDepthImage(TextureLayout_DepthAttachment);
        else if (renderPassAD->depthAttachment.texture != nullptr)
            renderPassAD->depthAttachment.texture->SetLayout(TextureLayout_DepthAttachment);

        // start render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPassPD->renderPass;
        renderPassInfo.framebuffer = renderPassPD->useSwapChain ? renderPassPD->frameBuffers[m_frameSwapImage] : renderPassPD->frameBuffers[0];
        ivec2 size = renderPassPD->useSwapChain ? GetSwapChainImageSize() : renderPassAD->size;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { (u32)size.x, (u32)size.y };

        renderPassInfo.clearValueCount = static_cast<u32>(renderPassPD->clearValues.size());
        renderPassInfo.pClearValues = renderPassPD->clearValues.data();
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // update the view UBO if this is a new view/aspectRatio combo
        auto view = renderPass->GetView();
        auto aspectRatio = renderPass->GetAspectRatio();
        if (view && view != m_boundView && aspectRatio != m_boundViewAspectRatio)
        {
            auto viewUBOInstance = ShaderManager::Instance().FindUBO("UBO_View")->dynamicInstance;
            UBO_View viewData;
            view->InitUBOView(viewData, aspectRatio);
            UpdateUBOInstance(viewUBOInstance, &viewData, sizeof(viewData), true);

            m_boundView = view;
            m_boundViewAspectRatio = aspectRatio;
        }
    }

    // no bound materials at the start of a render pass
    m_boundMaterial = nullptr;
}

#if PROFILING_ENABLED
void GIL::createTimeQueries()
{
    // Create query pool2
    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = MaxProfileTimestamps; // Two timestamps per query
    vkCreateQueryPool(m_device, &createInfo, nullptr, &m_queryPool);
}
u32 GIL::AddGpuTimeQuery()
{
    auto& commandBuffer = m_commandBuffers[m_currentFrame];
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, m_queryPool, m_queryIdx);
    return m_queryIdx++;
}

void GIL::GetGpuTimeQueryResults(u64*& buffer, int& count)
{
    if (m_queryIdx > 0)
    {
        vkGetQueryPoolResults(m_device, m_queryPool, 0, m_queryIdx, sizeof(uint64_t) * m_queryIdx, m_queryResults, sizeof(uint64_t), VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT);
    }
    buffer = m_queryResults;
    count = m_queryIdx;
    m_queryIdx = 0;
}
#endif

void GIL::ResizeSwapChain(ivec2 newsize)
{
    m_swapChainImageSize = newsize;
    cleanupSwapChain();
    recreateSwapChain();
}
