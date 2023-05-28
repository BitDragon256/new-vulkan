#include "render.h"

#include <vector>
#include <map>
#include <set>

#include "logger.h"
#include "vulkan_helpers.h"

NVE_RESULT Renderer::init(RenderConfig config)
{
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
    
    return NVE_SUCCESS;
}

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

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

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
        imageViewCI.format = swapChainImageFormat;
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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    {
        auto res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        log_cond_err(res == VK_SUCCESS, "failed to create pipeline layout");
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

    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_render_pass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
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

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

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
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
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
}

NVE_RESULT Renderer::record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    auto res = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording");
}

NVE_RESULT Renderer::render()
{
    if (glfwWindowShouldClose(m_window))
    {
        clean_up();
        return NVE_SUCCESS;
    }

    glfwPollEvents();
}
void Renderer::clean_up()
{
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