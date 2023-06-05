#include "render.h"

#include <vector>
#include <map>
#include <set>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include "logger.h"
#include "vulkan_helpers.h"

// ---------------------------------------
// RENDERER
// ---------------------------------------

// PUBLIC METHODS

NVE_RESULT Renderer::init(RenderConfig config)
{
    m_config = config;

    glfwInit();
    log_cond(create_window(config.width, config.height, config.title) == NVE_SUCCESS, "window created");
    log_cond(create_instance() == NVE_SUCCESS, "instance created");
    log_cond(get_surface() == NVE_SUCCESS, "surface created");
    log_cond(get_physical_device() == NVE_SUCCESS, "found physical device");
    log_cond(create_device() == NVE_SUCCESS, "logical device created");
    log_cond(create_swapchain() == NVE_SUCCESS, "swapchain created");
    log_cond(create_swapchain_image_views() == NVE_SUCCESS, "swapchain image views created");
    log_cond(create_render_pass() == NVE_SUCCESS, "render pass created");
    log_cond(create_graphics_pipeline() == NVE_SUCCESS, "graphics pipeline created");
    log_cond(create_framebuffers() == NVE_SUCCESS, "framebuffers created");
    log_cond(create_commandpool() == NVE_SUCCESS, "command pool created");
    log_cond(create_commandbuffer() == NVE_SUCCESS, "command buffer created");
    log_cond(create_sync_objects() == NVE_SUCCESS, "sync objects created");

    log_cond(init_vertex_buffer() == NVE_SUCCESS, "vertex buffer initialized");
    log_cond(init_index_buffer() == NVE_SUCCESS, "index buffer initialized");

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
NVE_RESULT Renderer::set_vertices(const std::vector<Vertex>& vertices)
{
    if (m_config.dataMode != RenderConfig::TestTri)
    {
        m_vertices = vertices;
        m_vertexBuffer.set(m_vertices);
    }

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::set_indices(const std::vector<Index>& indices)
{
    if (m_config.dataMode >= RenderConfig::Indexed)
    {
        m_indices = indices;
        m_indexBuffer.set(m_indices);
    }

    return NVE_SUCCESS;
}

// PRIVATE METHODS

NVE_RESULT Renderer::create_instance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "New Vulkan Engine Dev App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "New Vulkan Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceCI = {};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCI.pApplicationInfo = &appInfo;

    instanceCI.enabledLayerCount = static_cast<uint32_t>(m_config.enabledValidationLayers.size());
    instanceCI.ppEnabledLayerNames = m_config.enabledValidationLayers.data();
    
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    instanceCI.enabledExtensionCount = glfwExtensionCount;
    instanceCI.ppEnabledExtensionNames = glfwExtensions;
    
    VkResult res = vkCreateInstance(&instanceCI, nullptr, &m_instance);
    log_cond_err(res == VK_SUCCESS, "instance creation failed");
    
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

    // TODO validation layers

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
NVE_RESULT Renderer::create_graphics_pipeline()
{
    auto vertShaderCode = read_file("X:/Dev/new-vulkan-engine/shaders/vert.spv");
    auto fragShaderCode = read_file("X:/Dev/new-vulkan-engine/shaders/frag.spv");

    VkShaderModule vertShaderModule = create_shader_module(vertShaderCode, m_device);
    VkShaderModule fragShaderModule = create_shader_module(fragShaderCode, m_device);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // -------------------------------------------

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // -------------------------------------------

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // -------------------------------------------

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // -------------------------------------------

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // -------------------------------------------

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // -------------------------------------------

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

    // -------------------------------------------

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // -------------------------------------------

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    {
        auto res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        log_cond_err(res == VK_SUCCESS, "failed to create pipeline layout");
    }

    // -------------------------------------------

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    {
        auto res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
        log_cond_err(res == VK_SUCCESS, "failed to create graphics pipeline");
    }

    // -------------------------------------------

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

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

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

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
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    auto res = vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer);
    log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_sync_objects()
{
    VkSemaphoreCreateInfo semCI = {};
    semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCI = {};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    auto res = VK_SUCCESS;

    res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_imageAvailableSemaphore);
    log_cond_err(res == VK_SUCCESS, "failed to create imageAvailable semaphore");

    res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_renderFinishedSemaphore);
    log_cond_err(res == VK_SUCCESS, "failed to create renderFinished semaphore");

    res = vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFence);
    log_cond_err(res == VK_SUCCESS, "failed to create inFlight fence");

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::init_vertex_buffer()
{
    BufferConfig config;
    config.device = m_device;
    config.physicalDevice = m_physicalDevice;
    config.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    config.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    config.useStagedBuffer = true;

    m_vertexBuffer.initialize(config);
    
    return NVE_SUCCESS;
}
NVE_RESULT Renderer::init_index_buffer()
{
    BufferConfig config;
    config.device = m_device;
    config.physicalDevice = m_physicalDevice;
    config.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    config.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    config.useStagedBuffer = true;

    m_indexBuffer.initialize(config);
    
    return NVE_SUCCESS;
}

NVE_RESULT Renderer::record_main_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBI = {};
    commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBI.flags = 0; // Optional
    commandBufferBI.pInheritanceInfo = nullptr; // Optional

    {
        auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
        log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording");
    }

    // -------------------------------------------

    VkRenderPassBeginInfo renderPassBI = {};
    renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBI.renderPass = m_renderPass;
    renderPassBI.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderPassBI.renderArea.offset = { 0, 0 };
    renderPassBI.renderArea.extent = m_swapchainExtent;

    VkClearValue clearValue = { {{ 0.f, 0.f, 0.f, 1.f }} };
    renderPassBI.clearValueCount = 1;
    renderPassBI.pClearValues = &clearValue;

    vkCmdBeginRenderPass(m_commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    // -------------------------------------------

    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    // -------------------------------------------

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchainExtent.width);
    viewport.height = static_cast<float>(m_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // -------------------------------------------
    
    VkDeviceSize offsets[] = { 0 };
    VkBuffer vertexBuffers[] = { m_vertexBuffer.m_buffer };

    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);

    // -------------------------------------------

    if (m_config.dataMode == RenderConfig::Indexed)
    vkCmdBindIndexBuffer(m_commandBuffer, m_indexBuffer.m_buffer, 0, NVE_INDEX_TYPE);

    // -------------------------------------------

    switch (m_config.dataMode)
    {
    case RenderConfig::TestTri:
        vkCmdDraw(m_commandBuffer, 3, 1, 0, 0);
    case RenderConfig::VertexOnly:
        vkCmdDraw(m_commandBuffer, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);
    case RenderConfig::Indexed:
        vkCmdDrawIndexed(m_commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    // -------------------------------------------

    vkCmdEndRenderPass(m_commandBuffer);

    {
        auto res = vkEndCommandBuffer(m_commandBuffer);
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

    auto res = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence);
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
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);

    // Acquire an image from the swap chain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Record a command buffer which draws the scene onto that image
    vkResetCommandBuffer(m_commandBuffer, 0);
    record_main_command_buffer(m_commandBuffer, imageIndex);

    imgui_demo_draws(imageIndex);

    std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphore };
    std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphore };

    // Submit the recorded command buffers
    std::vector<VkCommandBuffer> commandBuffers = { m_commandBuffer, m_imgui_commandBuffers[imageIndex]};
    submit_command_buffers(commandBuffers, waitSemaphores, signalSemaphores);

    // Present the swap chain image
    present_swapchain_image(m_swapchain, imageIndex, signalSemaphores);

    return NVE_SUCCESS;
}

VkCommandBuffer Renderer::begin_single_time_cmd_buffer(VkCommandPool cmdPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void Renderer::end_single_time_cmd_buffer(VkCommandBuffer commandBuffer, VkCommandPool cmdPool)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, cmdPool, 1, &commandBuffer);
}

void Renderer::clean_up()
{
    vkDeviceWaitIdle(m_device);

    imgui_cleanup();

    m_vertexBuffer.destroy();
    m_indexBuffer.destroy();

    vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
    vkDestroyFence(m_device, m_inFlightFence, nullptr);

    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    for (auto imageView : m_swapchainImageViews)
        vkDestroyImageView(m_device, imageView, nullptr);
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

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

    VkImageView attachment[1];
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
    VkCommandBuffer commandBuffer = begin_single_time_cmd_buffer(m_imgui_commandPool);
    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
    end_single_time_cmd_buffer(commandBuffer, m_imgui_commandPool);

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

void Renderer::imgui_demo_draws(uint32_t imageIndex)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
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
std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}