#include "render.h"

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "logger.h"
#include "vulkan_helpers.h"

// ---------------------------------------
// RENDERER
// ---------------------------------------

// PUBLIC METHODS

NVE_RESULT Renderer::init(RenderConfig config)
{
    m_config = config;

    // add_descriptors();

    glfwInit();
    log_cond(create_window(config.width, config.height, config.title) == NVE_SUCCESS, "window created");
    log_cond(create_instance() == NVE_SUCCESS, "instance created");
    // log_cond(create_debug_messenger() == NVE_SUCCESS, "debug messenger created");
    log_cond(get_surface() == NVE_SUCCESS, "surface created");
    log_cond(get_physical_device() == NVE_SUCCESS, "found physical device");
    log_cond(create_device() == NVE_SUCCESS, "logical device created");
    log_cond(create_swapchain() == NVE_SUCCESS, "swapchain created");
    log_cond(create_swapchain_image_views() == NVE_SUCCESS, "swapchain image views created");
    log_cond(create_render_pass() == NVE_SUCCESS, "render pass created");
    log_cond(create_framebuffers() == NVE_SUCCESS, "framebuffers created");
    log_cond(create_commandpool() == NVE_SUCCESS, "command pool created");
    log_cond(create_commandbuffer() == NVE_SUCCESS, "command buffer created");
    log_cond(create_sync_objects() == NVE_SUCCESS, "sync objects created");

    initialize_geometry_handlers(); log("geometry pipelines created");

    // imgui
    imgui_init();
    
    return NVE_SUCCESS;
}
NVE_RESULT Renderer::render()
{
    if (glfwWindowShouldClose(m_window))
    {
        clean_up();
        return NVE_RENDER_EXIT_SUCCESS;
    }

    glfwPollEvents();

    draw_frame();

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::set_active_camera(Camera* camera)
{
    if (!camera)
        return NVE_FAILURE;
    m_activeCamera = camera;
    m_activeCamera->m_extent.x = m_swapchainExtent.width;
    m_activeCamera->m_extent.y = m_swapchainExtent.height;
    return NVE_SUCCESS;
}

int Renderer::get_key(int key)
{
    return glfwGetKey(m_window, key);
}

// PRIVATE METHODS

NVE_RESULT Renderer::create_instance()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "New Vulkan Engine Dev App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "New Vulkan Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    appInfo.pNext = nullptr;

    VkInstanceCreateInfo instanceCI = {};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appInfo;
    
    if (m_config.enableValidationLayers)
        m_config.enabledInstanceLayers.push_back("VK_LAYER_KHRONOS_validation");

    instanceCI.enabledLayerCount = static_cast<uint32_t>(m_config.enabledInstanceLayers.size());
    instanceCI.ppEnabledLayerNames = m_config.enabledInstanceLayers.data();
    
    //VkDebugUtilsMessengerCreateInfoEXT debugMessengerCI = {};
    //populate_debug_messenger_create_info(debugMessengerCI);
    instanceCI.pNext = nullptr;//&debugMessengerCI;
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    log("acquiring GLFW extensions...");
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    log("acquired GLFW extensions");
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    //std::vector<const char*> extensions{ "VK_KHR_surface" };

    if (m_config.enableValidationLayers)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCI.ppEnabledExtensionNames = extensions.data();
    instanceCI.flags = 0;
    
    log("creating instance...");
    VkResult res = vkCreateInstance(&instanceCI, nullptr, &m_instance);
    if (res != VK_SUCCESS)
    {
        log("instance creation failed: " + res);
        return NVE_FAILURE;
    }
    
    return NVE_SUCCESS;
}
NVE_RESULT Renderer::get_physical_device()
{
    // get all available physical devices
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    log_cond_err(deviceCount != 0, "no valid physical devices found");
    std::vector<VkPhysicalDevice> availableDevices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, availableDevices.data());
    
#ifdef RATE_PHYSICAL_DEVICES

    std::multimap<uint32_t, VkPhysicalDevice> ratedDevices;
    for (const auto& device : availableDevices)
    {
        uint32_t score = rate_device_suitability(device, m_surface);
        ratedDevices.insert(std::make_pair(score, device));
    }

    log_cond_err(ratedDevices.rbegin()->first > 0, "no acceptable physical device found");

    m_physicalDevice = ratedDevices.rbegin()->second;

#else
    
    m_physicalDevice = VK_NULL_HANDLE;
    for (const auto& device : availableDevices) 
    {
        if (is_device_suitable(device, m_surface))
        {
            m_physicalDevice = device;
            break;
        }
    }
    log_cond_err(m_physicalDevice != VK_NULL_HANDLE, "no acceptable physical device found");
    
#endif
    
    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_debug_messenger()
{
    if (!m_config.enableValidationLayers)
        return NVE_SUCCESS;
        
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCI = {};
    populate_debug_messenger_create_info(debugMessengerCI);
    
    auto res = vkCreateDebugUtilsMessengerEXT(m_instance, &debugMessengerCI, nullptr, &m_debugMessenger);
    log_cond_err(res == VK_SUCCESS, "failed to create debug messenger");
    
    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_device()
{
    m_queueFamilyIndices = find_queue_families(m_physicalDevice, m_surface);

    // specify queues
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs = {};
    std::set<uint32_t> uniqueQueueFamilyIndices = { m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentationFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        VkDeviceQueueCreateInfo deviceQueueCI = {};

        deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCI.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCI.queueCount = 1;
        deviceQueueCI.pQueuePriorities = &queuePriority;
        deviceQueueCIs.push_back(deviceQueueCI);
    }

    // physical device stuff
    VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

    // m_device creation
    VkDeviceCreateInfo deviceCI = {};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCIs.size());
    deviceCI.pQueueCreateInfos = deviceQueueCIs.data();
    deviceCI.pEnabledFeatures = &physicalDeviceFeatures;
    deviceCI.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCI.ppEnabledExtensionNames = deviceExtensions.data();

    VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParametersFeatures = {};
    shaderDrawParametersFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    shaderDrawParametersFeatures.pNext = nullptr;
    shaderDrawParametersFeatures.shaderDrawParameters = VK_TRUE;

    deviceCI.pNext = &shaderDrawParametersFeatures;

    auto res = vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device);
    log_cond_err(res == VK_SUCCESS, "physical device creation failed");

    vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue);

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_window(int width, int height, std::string title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::get_surface()
{
    auto res = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
    log_cond_err(res == VK_SUCCESS, "creation of window surface failed");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_swapchain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(m_physicalDevice, m_surface);

    VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities, m_window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchainCI = {};
    swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCI.surface = m_surface;
    swapchainCI.minImageCount = imageCount;
    swapchainCI.imageFormat = surfaceFormat.format;
    swapchainCI.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCI.imageExtent = extent;
    swapchainCI.imageArrayLayers = 1;
    swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t queueFamilyIndices[] = { m_queueFamilyIndices.graphicsFamily.value(), m_queueFamilyIndices.presentationFamily.value() };

    if (m_queueFamilyIndices.graphicsFamily != m_queueFamilyIndices.presentationFamily) {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCI.queueFamilyIndexCount = 2;
        swapchainCI.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCI.queueFamilyIndexCount = 0; // Optional
        swapchainCI.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchainCI.preTransform = swapChainSupport.capabilities.currentTransform;
    swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCI.presentMode = presentMode;
    swapchainCI.clipped = VK_TRUE;
    swapchainCI.oldSwapchain = VK_NULL_HANDLE;

    auto res = vkCreateSwapchainKHR(m_device, &swapchainCI, nullptr, &m_swapchain);

    log_cond_err(res == VK_SUCCESS, "creation of swapchain failed");

    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_swapchain_image_views()
{
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo imageViewCI = {};
        imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCI.image = m_swapchainImages[i];
        imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCI.format = m_swapchainImageFormat;
        imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCI.subresourceRange.baseMipLevel = 0;
        imageViewCI.subresourceRange.levelCount = 1;
        imageViewCI.subresourceRange.baseArrayLayer = 0;
        imageViewCI.subresourceRange.layerCount = 1;

        auto res = vkCreateImageView(m_device, &imageViewCI, nullptr, &m_swapchainImageViews[i]);
        log_cond_err(res == VK_SUCCESS, "failed to create image view no " + i);
    }

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_render_pass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    uint32_t subpassCount = geometry_handler_subpass_count();
    std::vector<VkSubpassDescription> subpasses(subpassCount);

    for (VkSubpassDescription& subpass : subpasses)
    {
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
    }

    VkSubpassDependency firstDependency = {};
    firstDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    firstDependency.dstSubpass = 0;
    firstDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    firstDependency.srcAccessMask = 0;
    firstDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    firstDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::vector<VkSubpassDependency> dependencies(subpassCount - 1);
    uint32_t subpassIndex = 1;
    for (VkSubpassDependency& dependency : dependencies)
    {
        dependency.srcSubpass = subpassIndex;
        dependency.dstSubpass = subpassIndex + 1;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        subpassIndex++;
    }

    dependencies.insert(dependencies.begin(), firstDependency);

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = subpassCount;
    renderPassInfo.pSubpasses = subpasses.data();

    renderPassInfo.dependencyCount = subpassCount;
    renderPassInfo.pDependencies = dependencies.data();

    auto res = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    log_cond_err(res == VK_SUCCESS, "failed to create render pass");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_framebuffers()
{
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;

        auto res = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]);
        log_cond_err(res == VK_SUCCESS, "failed to create framebuffer no " + i);
    }

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_commandpool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

    auto res = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

    log_cond_err(res == VK_SUCCESS, "failed to create command pool");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_commandbuffer()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_swapchainFramebuffers.size();

    auto res = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
    log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_sync_objects()
{
    for (size_t frame = 0; frame < m_swapchainFramebuffers.size(); frame++)
    {
        VkSemaphoreCreateInfo semCI = {};
        semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceCI = {};
        fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        auto res = VK_SUCCESS;

        res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_imageAvailableSemaphores[frame]);
        log_cond_err(res == VK_SUCCESS, "failed to create imageAvailable semaphore");

        res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_renderFinishedSemaphores[frame]);
        log_cond_err(res == VK_SUCCESS, "failed to create renderFinished semaphore");

        res = vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFences[frame]);
        log_cond_err(res == VK_SUCCESS, "failed to create inFlight fence");
    }

    return NVE_SUCCESS;
}

void Renderer::initialize_geometry_handlers()
{
    StaticGeometryHandlerVulkanObjects vulkanObjects;
    vulkanObjects.device = m_device;
    vulkanObjects.physicalDevice = m_physicalDevice;
    vulkanObjects.commandPool = m_commandPool;
    vulkanObjects.renderPass = m_renderPass;
    vulkanObjects.transferQueue = m_graphicsQueue;

    vulkanObjects.swapchainExtent = m_swapchainExtent;
    vulkanObjects.framebuffers = m_swapchainFramebuffers;
    vulkanObjects.firstSubpass = 0;
    m_staticGeometryHandler.initialize(vulkanObjects);
}
void Renderer::create_geometry_pipelines()
{
    
}

void Renderer::destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

NVE_RESULT Renderer::record_main_command_buffer(uint32_t frame)
{
    VkCommandBuffer commandBuffer = m_commandBuffers[frame];

    // -------------------------------------------

    VkCommandBufferBeginInfo commandBufferBI = {};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBI.pNext = nullptr;
    commandBufferBI.flags = 0;
    commandBufferBI.pInheritanceInfo = nullptr;

    {
        auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
        log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording");
    }

    // -------------------------------------------

    VkRenderPassBeginInfo renderPassBI = {};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = m_renderPass;
    renderPassBI.framebuffer = m_swapchainFramebuffers[frame];
    renderPassBI.renderArea.offset = { 0, 0 };
    renderPassBI.renderArea.extent = m_swapchainExtent;

    VkClearValue clearValue = { {{ m_config.clearColor.x / 255.f, m_config.clearColor.y / 255.f, m_config.clearColor.z / 255.f, 1.f }} };
    renderPassBI.clearValueCount = 1;
    renderPassBI.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    // -------------------------------------------

    uint32_t subpassCount = m_staticGeometryHandler.subpass_count();
    for (uint32_t subpass = 0; subpass < subpassCount; subpass++)
    {

    }

    // -------------------------------------------

    vkCmdEndRenderPass(commandBuffer);

    {
        auto res = vkEndCommandBuffer(commandBuffer);
        log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording");
    }

    // -------------------------------------------

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::submit_command_buffers(std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSems, std::vector<VkSemaphore> signalSems)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
    submitInfo.pWaitSemaphores = waitSems.data();
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSems.size());
    submitInfo.pSignalSemaphores = signalSems.data();

    auto res = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_frame]);
    log_cond_err(res == VK_SUCCESS, "failed to submit command buffers to graphics queue");

    return NVE_SUCCESS;
}
void Renderer::present_swapchain_image(VkSwapchainKHR swapchain, uint32_t imageIndex, std::vector<VkSemaphore> signalSems)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSems.size());
    presentInfo.pWaitSemaphores = signalSems.data();

    VkSwapchainKHR swapChains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_presentationQueue, &presentInfo);
}

NVE_RESULT Renderer::draw_frame()
{
    // Wait for the previous frame to finish
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFences[m_frame]);

    // Acquire an image from the swap chain
    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_frame], VK_NULL_HANDLE, &m_frame);

    // Record a command buffer which draws the scene onto that image
    vkResetCommandBuffer(m_commandBuffers[m_frame], 0);
    record_main_command_buffer(m_frame);

    if (!m_imguiDraw)
        gui_begin();

    imgui_draw(m_frame);
    m_imguiDraw = false;

    std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphores[m_frame] };
    std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphores[m_frame] };

    // Submit the recorded command buffers
    std::vector<VkCommandBuffer> commandBuffers = { m_commandBuffers[m_frame], m_imgui_commandBuffers[m_frame]};
    submit_command_buffers(commandBuffers, waitSemaphores, signalSemaphores);

    // Present the swap chain image
    present_swapchain_image(m_swapchain, m_frame, signalSemaphores);

    return NVE_SUCCESS;
}

void Renderer::clean_up()
{
    vkDeviceWaitIdle(m_device);

    imgui_cleanup();

    // vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
    // vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    for (uint32_t frame = 0; frame < m_swapchainFramebuffers.size(); frame++)
    {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[static_cast<size_t>(frame)], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[static_cast<size_t>(frame)], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[static_cast<size_t>(frame)], nullptr);
    }

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (auto imageView : m_swapchainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    
    // if (m_config.enableValidationLayers)
    //     destroy_debug_messenger(m_instance, m_debugMessenger, nullptr);
    
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

uint32_t Renderer::geometry_handler_subpass_count()
{
    return m_staticGeometryHandler.subpass_count();
}

//void Renderer::add_descriptors()
//{
//    if (m_config.useModelHandler)
//    {
//        VkDescriptorSetLayoutBinding binding = {};
//        binding.binding = NVE_MODEL_INFO_BUFFER_BINDING;
//        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//        binding.descriptorCount = NVE_MAX_MODEL_INFO_COUNT;
//
//        m_descriptors.push_back(binding);
//    }
//}
//void Renderer::update_model_info_descriptor_set()
//{
//    VkWriteDescriptorSet write = {};
//    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//    write.dstSet = m_descriptorSet;
//    write.dstBinding = NVE_MODEL_INFO_BUFFER_BINDING;
//    write.dstArrayElement = 0;
//    write.descriptorCount = 1;
//    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//
//    VkDescriptorBufferInfo bufferInfo = {};
//    bufferInfo.buffer = m_pModelHandler->model_buffer();
//    bufferInfo.offset = 0;
//    bufferInfo.range = VK_WHOLE_SIZE;
//
//    write.pBufferInfo = &bufferInfo;
//
//    vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
//}

// ---------------------------------------
// GUI
// ---------------------------------------

NVE_RESULT Renderer::imgui_init()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    imgui_create_descriptor_pool();
    imgui_create_render_pass();
    imgui_create_framebuffers();
    imgui_create_command_pool();
    imgui_create_command_buffers();

    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_instance;
    initInfo.PhysicalDevice = m_physicalDevice;
    initInfo.Device = m_device;
    initInfo.QueueFamily = 42; // is it working?
    initInfo.Queue = m_graphicsQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = m_imgui_descriptorPool;
    initInfo.Subpass = 0;

    uint32_t imageCount = static_cast<uint32_t>(m_swapchainImages.size());

    initInfo.MinImageCount = imageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    //initInfo.CheckVkResultFn = imgui_error_handle;
    ImGui_ImplVulkan_Init(&initInfo, m_imgui_renderPass);

    imgui_upload_fonts();
    m_imguiDraw = false;

    log("imgui initialized");

    return NVE_SUCCESS;
}

void imgui_error_handle(VkResult err)
{
    throw std::runtime_error("graphical user interface error: " + err);
}
NVE_RESULT Renderer::imgui_create_render_pass()
{
    VkAttachmentDescription attachment = {};
    attachment.format = m_swapchainImageFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachment = {};
    colorAttachment.attachment = 0;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachment;

    // an external subpass dependency, so that the main subpass with the geometry is rendered before the gui
    // this is basically a driver-handled pipeline barrier
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;  // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCI = {};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &attachment;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpass;
    renderPassCI.dependencyCount = 1;
    renderPassCI.pDependencies = &dependency;

    auto res = vkCreateRenderPass(m_device, &renderPassCI, nullptr, &m_imgui_renderPass);
    log_cond_err(res == VK_SUCCESS, "failed to create imgui render pass");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_framebuffers()
{
    m_imgui_framebuffers.resize(m_swapchainImages.size());

    VkImageView attachment[1] = {};
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = m_imgui_renderPass;
    info.attachmentCount = 1;
    info.pAttachments = attachment;
    info.width = m_swapchainExtent.width;
    info.height = m_swapchainExtent.height;
    info.layers = 1;
    for (uint32_t i = 0; i < m_swapchainImages.size(); i++)
    {
        attachment[0] = m_swapchainImageViews[i];
        auto res = vkCreateFramebuffer(m_device, &info, nullptr, &m_imgui_framebuffers[i]);
        log_cond_err(res == VK_SUCCESS, "failed to create imgui framebuffer no " + i);
    }

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_descriptor_pool()
{
    // array from https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo descPoolCI = {};
    descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descPoolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descPoolCI.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
    descPoolCI.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(poolSizes));
    descPoolCI.pPoolSizes = poolSizes;

    auto res = vkCreateDescriptorPool(m_device, &descPoolCI, nullptr, &m_imgui_descriptorPool);
    log_cond_err(res == VK_SUCCESS, "failed to create imgui descriptor pool");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_upload_fonts()
{
    VkCommandBuffer commandBuffer = begin_single_time_cmd_buffer(m_imgui_commandPool, m_device);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    end_single_time_cmd_buffer(commandBuffer, m_imgui_commandPool, m_device, m_graphicsQueue);

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_command_pool()
{
    VkCommandPoolCreateInfo cmdPoolCI = {};
    cmdPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolCI.queueFamilyIndex = m_queueFamilyIndices.graphicsFamily.value();

    auto res = vkCreateCommandPool(m_device, &cmdPoolCI, nullptr, &m_imgui_commandPool);
    log_cond_err(res == VK_SUCCESS, "failed to create imgui command pool");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::imgui_create_command_buffers()
{
    m_imgui_commandBuffers.resize(m_swapchainImages.size());

    VkCommandBufferAllocateInfo cmdBufferAI = {};
    cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAI.commandPool = m_imgui_commandPool;
    cmdBufferAI.commandBufferCount = static_cast<uint32_t>(m_swapchainImages.size());
    cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    auto res = vkAllocateCommandBuffers(m_device, &cmdBufferAI, m_imgui_commandBuffers.data());
    log_cond_err(res == VK_SUCCESS, "failed to create imgui command buffers");

    return NVE_SUCCESS;
}

void Renderer::gui_begin()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    m_imguiDraw = true;
}
void Renderer::imgui_draw(uint32_t imageIndex)
{
    //ImGui::ShowDemoWindow();

    ImGui::Render();

    {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        auto res = vkBeginCommandBuffer(m_imgui_commandBuffers[imageIndex], &info);
        log_cond_err(res == VK_SUCCESS, "failed to begin imgui command buffer on image index " + imageIndex);
    }

    {
        VkRenderPassBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.renderPass = m_imgui_renderPass;
        info.framebuffer = m_imgui_framebuffers[imageIndex];
        info.renderArea.extent.width = m_swapchainExtent.width;
        info.renderArea.extent.height = m_swapchainExtent.height;
        info.clearValueCount = 1;
        info.pClearValues = &m_imgui_clearColor;
        vkCmdBeginRenderPass(m_imgui_commandBuffers[imageIndex], &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_imgui_commandBuffers[imageIndex]);
    
    vkCmdEndRenderPass(m_imgui_commandBuffers[imageIndex]);
    auto res = vkEndCommandBuffer(m_imgui_commandBuffers[imageIndex]);
    log_cond_err(res == VK_SUCCESS, "failed to end imgui command buffer no " + imageIndex);
}
void Renderer::imgui_cleanup()
{
    for (auto fb : m_imgui_framebuffers)
        vkDestroyFramebuffer(m_device, fb, nullptr);

    vkDestroyRenderPass(m_device, m_imgui_renderPass, nullptr);
    vkFreeCommandBuffers(m_device, m_imgui_commandPool, static_cast<uint32_t>(m_imgui_commandBuffers.size()), m_imgui_commandBuffers.data());
    vkDestroyCommandPool(m_device, m_imgui_commandPool, nullptr);

    // Resources to destroy when the program ends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(m_device, m_imgui_descriptorPool, nullptr);
}

// ---------------------------------------
// VERTEX
// ---------------------------------------

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}
std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> attributeDescriptions = {};

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
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
}

// ---------------------------------------
// CAMERA
// ---------------------------------------
Camera::Camera() :
    m_position(0), m_rotation(0), m_fov(90), m_nearPlane(0.01f), m_farPlane(10.f), m_extent(1080, 1920), renderer(nullptr)
{}
glm::mat4 Camera::view_matrix()
{
    return glm::lookAt(m_position, m_position + glm::rotate(glm::qua(glm::radians(m_rotation)), VECTOR_FORWARD), VECTOR_UP);
}
glm::mat4 Camera::projection_matrix()
{
    return glm::perspective(glm::radians(m_fov), m_extent.x / m_extent.y, m_nearPlane, m_farPlane);
}
