#include "render.h"

#include <vector>
#include <map>
#include <set>

#include "logger.h"
#include "vulkan_helpers.h"

// ---------------------------------------
// RENDERER
// ---------------------------------------

// PUBLIC METHODS

NVE_RESULT Renderer::init(RenderConfig config)
{
    m_config = config;

    m_vertexBufferCreated = false;

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
    if (m_config.vertexOnlyMode || m_config.vertexIndexMode)
    {
        m_vertices = vertices;
        log_cond(create_vertex_buffer(m_vertices.size()) == NVE_SUCCESS, "vertex buffer created");
    }

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::set_indices(const std::vector<Index>& indices)
{
    if (m_config.vertexIndexMode)
    {
        m_indices = indices;
    }
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
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice, m_surface);

    // specify queues
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs = {};
    std::set<uint32_t> uniqueQueueFamilyIndices = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentationFamily.value() };

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

    vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue);

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

    QueueFamilyIndices indices = find_queue_families(m_physicalDevice, m_surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentationFamily.value() };

    if (indices.graphicsFamily != indices.presentationFamily) {
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
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice, m_surface);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

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
NVE_RESULT Renderer::create_vertex_buffer(uint32_t size)
{
    if (m_vertices.size() == 0)
        return NVE_FAILURE;

    if (m_vertexBufferCreated)
    {
        vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    }

    VkBufferCreateInfo bufferCI{};
    bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCI.size = sizeof(m_vertices[0]) * size;
    bufferCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    {
        auto res = vkCreateBuffer(m_device, &bufferCI, nullptr, &m_vertexBuffer);
        log_cond_err(res == VK_SUCCESS, "failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(m_physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        auto res = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexBufferMemory);
        log_cond_err(res == VK_SUCCESS, "failed to allocate vertex buffer memory");
    }

    vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

    void* data;
    vkMapMemory(m_device, m_vertexBufferMemory, 0, bufferCI.size, 0, &data);
    memcpy(data, m_vertices.data(), (size_t)bufferCI.size);
    vkUnmapMemory(m_device, m_vertexBufferMemory);

    m_vertexBufferCreated = true;

    return NVE_SUCCESS;
}

NVE_RESULT Renderer::record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
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
    
    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);

    // -------------------------------------------

    vkCmdDraw(m_commandBuffer, static_cast<uint32_t>(m_vertices.size()), 1, 0, 0);

    // -------------------------------------------

    vkCmdEndRenderPass(m_commandBuffer);

    {
        auto res = vkEndCommandBuffer(m_commandBuffer);
        log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording");
    }

    // -------------------------------------------

    return NVE_SUCCESS;
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
    record_command_buffer(m_commandBuffer, imageIndex);

    // Submit the recorded command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffer;

    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    auto res = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence);
    log_cond_err(res == VK_SUCCESS, "failed to submit command buffer to graphics queue");

    // Present the swap chain image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(m_presentationQueue, &presentInfo);

    return NVE_SUCCESS;
}

void Renderer::clean_up()
{
    vkDeviceWaitIdle(m_device);

    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

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