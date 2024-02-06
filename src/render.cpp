#include "render.h"

#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vk_enum_string_helper.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "logger.h"
#include "vulkan_helpers.h"
#include "material.h"

#ifdef RENDER_PROFILER
#define PROFILE_START(X) m_profiler.start_measure(X);
#define PROFILE_END(X) m_profiler.end_measure(X, true);
#else
#define PROFILE_START(X)
#define PROFILE_END(X) 0.f;
#endif

// ---------------------------------------
// NON-MEMBER FUNCTIONS
// ---------------------------------------

template<typename T>
void append_vector(std::vector<T>& origin, std::vector<T>& appendage)
{
      if (appendage.empty())
            return;
      origin.insert(origin.end(), appendage.begin(), appendage.end());
}


// ---------------------------------------
// RENDERER
// ---------------------------------------

// PUBLIC METHODS

Renderer::Renderer() :
      m_ecs{ this }, m_initialized{ false }
{}
Renderer::~Renderer()
{
      clean_up();
}
NVE_RESULT Renderer::init(RenderConfig config)
{
      m_config = config;
      m_firstFrame = true;
      m_initialized = true;

      // add_descriptors();

      glfwInit();
      logger::log_cond(create_window(config.width, config.height, config.title) == NVE_SUCCESS, "window created");
      logger::log_cond(create_instance() == NVE_SUCCESS, "instance created");
      // logger::log_cond(create_debug_messenger() == NVE_SUCCESS, "debug messenger created");
      logger::log_cond(get_surface() == NVE_SUCCESS, "surface created");
      logger::log_cond(get_physical_device() == NVE_SUCCESS, "found physical device");
      logger::log_cond(create_device() == NVE_SUCCESS, "logical device created");
      logger::log_cond(create_swapchain() == NVE_SUCCESS, "swapchain created");
      logger::log_cond(create_swapchain_image_views() == NVE_SUCCESS, "swapchain image views created");
      create_depth_images();              logger::log("depth images created");
      logger::log_cond(create_render_pass() == NVE_SUCCESS, "render pass created");
      logger::log_cond(create_framebuffers() == NVE_SUCCESS, "framebuffers created");
      logger::log_cond(create_commandpool() == NVE_SUCCESS, "command pool created");
      logger::log_cond(create_commandbuffers() == NVE_SUCCESS, "command buffer created");
      logger::log_cond(create_sync_objects() == NVE_SUCCESS, "sync objects created");

      m_threadPool.initialize(m_threadCount);

      // TODO delete this
      Shader::s_device = m_device;

      create_descriptor_pool();           logger::log("descriptor pool created");

      initialize_geometry_handlers();     logger::log("geometry handlers initialized");

      // imgui
      imgui_init();

      m_frame = 0;

      m_deltaTime = 0;
      m_lastFrameTime = std::chrono::high_resolution_clock::now();

      m_avgRenderTime = 0;
      m_acquireImageTimeout = false;

      m_ecs.lock();
      m_guiManager.initialize(&m_ecs);

      init_default_camera();

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::render()
{
      PROFILE_START("total render time");
      PROFILE_START("glfw window should close poll");
      if (glfwWindowShouldClose(m_window))
      {
            return NVE_RENDER_EXIT_SUCCESS;
      }
      PROFILE_END("glfw window should close poll");

      std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
      m_deltaTime = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_lastFrameTime).count() / 1000000000.f;
      m_lastFrameTime = now;

      PROFILE_START("ecs update");
      if (m_firstFrame || m_config.autoECSUpdate)
      {
            m_ecs.unlock();
            m_ecs.update_systems(m_deltaTime);
            m_ecs.lock();
      }
      PROFILE_END("ecs update");

      PROFILE_START("glfw poll events");
      glfwPollEvents();
      PROFILE_END("glfw poll events");

      PROFILE_START("draw frame");
      draw_frame();
      PROFILE_END("draw frame");
      PROFILE_END("total render time");

      m_ecs.unlock();

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
Camera& Renderer::active_camera()
{
      return *m_activeCamera;
}

int Renderer::get_key(int key)
{
      return glfwGetKey(m_window, key);
}
int Renderer::get_mouse_button(int btn)
{
      return glfwGetMouseButton(m_window, btn);
}
Vector2 Renderer::get_mouse_pos()
{
      double xPos, yPos;
      glfwGetCursorPos(m_window, &xPos, &yPos);
      return { xPos, yPos };
}
Vector2 Renderer::mouse_to_screen(Vector2 mouse)
{
      return 2.f * Vector2 {
            mouse.x / m_swapchainExtent.width / m_guiManager.m_cut.x,
                  mouse.y / m_swapchainExtent.height / m_guiManager.m_cut.y
      } - Vector2(1.f);
}

void Renderer::set_light_pos(Vector3 pos)
{
      m_cameraPushConstant.lightPos = pos;
}

void Renderer::gizmos_draw_line(Vector3 start, Vector3 end, Color color, float width)
{
      m_gizmosHandler.draw_line(start, end, color, width);
}
void Renderer::gizmos_draw_ray(Vector3 start, Vector3 direction, Color color, float width)
{
      m_gizmosHandler.draw_ray(start, direction, color, width);
}


EntityId Renderer::create_empty_game_object()
{
      const auto entity = m_ecs.create_entity();
      m_ecs.add_component<Transform>(entity);
      return entity;
}
EntityId Renderer::create_default_model(DefaultModel::DefaultModel defaultModel)
{
      const auto entity = create_empty_game_object();
      auto& model = m_ecs.add_component<DynamicModel>(entity);
      model.load_mesh(s_defaultModelToPath[defaultModel]);
      return entity;
}

void Renderer::reload_pipelines()
{
      await_last_frame_render();
      create_geometry_pipelines();
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

      logger::log("acquiring GLFW extensions...");
      glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
      logger::log("acquired GLFW extensions");

      std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
      //std::vector<const char*> extensions{ "VK_KHR_surface" };

      if (m_config.enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

      instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
      instanceCI.ppEnabledExtensionNames = extensions.data();
      instanceCI.flags = 0;

      logger::log("creating instance...");
      VkResult res = vkCreateInstance(&instanceCI, nullptr, &m_instance);
      if (res != VK_SUCCESS)
      {
            logger::log("instance creation failed: " + std::string(string_VkResult(res)));
            return NVE_FAILURE;
      }

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::get_physical_device()
{
      // get all available physical devices
      uint32_t deviceCount;
      vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
      logger::log_cond_err(deviceCount != 0, "no valid physical devices found");
      std::vector<VkPhysicalDevice> availableDevices(deviceCount);
      vkEnumeratePhysicalDevices(m_instance, &deviceCount, availableDevices.data());

#ifdef RATE_PHYSICAL_DEVICES

      std::multimap<uint32_t, VkPhysicalDevice> ratedDevices;
      for (const auto& device : availableDevices)
      {
            uint32_t score = rate_device_suitability(device, m_surface);
            ratedDevices.insert(std::make_pair(score, device));
      }

      logger::log_cond_err(ratedDevices.rbegin()->first > 0, "no acceptable physical device found");

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
      logger::log_cond_err(m_physicalDevice != VK_NULL_HANDLE, "no acceptable physical device found");

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
      logger::log_cond_err(res == VK_SUCCESS, "failed to create debug messenger");

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
      physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

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
      logger::log_cond_err(res == VK_SUCCESS, "physical device creation failed");

      vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
      vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue);
      vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
      vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);

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
      logger::log_cond_err(res == VK_SUCCESS, "creation of window surface failed");

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

      logger::log_cond_err(res == VK_SUCCESS, "creation of swapchain failed");

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
            logger::log_cond_err(res == VK_SUCCESS, "failed to create image view no " + i);
      }

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_render_pass()
{
      if (!m_firstFrame)
            vkDestroyRenderPass(m_device, m_renderPass, nullptr);

      // color attachment

      VkAttachmentDescription colorAttachment = {};
      colorAttachment.format = m_swapchainImageFormat;
      colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      VkAttachmentReference colorAttachmentRef = {};
      colorAttachmentRef.attachment = 0;
      colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      // depth attachment

      VkAttachmentDescription depthAttachment = {};
      depthAttachment.flags = 0;
      depthAttachment.format = find_depth_format(m_physicalDevice);
      depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      VkAttachmentReference depthAttachmentRef = {};
      depthAttachmentRef.attachment = 1;
      depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      uint32_t subpassCount = geometry_handler_subpass_count();
      m_lastGeometryHandlerSubpassCount = subpassCount;
      std::vector<VkSubpassDescription> subpasses(subpassCount);

      for (VkSubpassDescription& subpass : subpasses)
      {
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
      }

      std::vector<VkSubpassDependency> dependencies;
      if (subpassCount > 1)
      {
            dependencies.resize(subpassCount - 1);
            uint32_t subpassIndex = 0;
            for (VkSubpassDependency& dependency : dependencies)
            {
                  dependency.srcSubpass = subpassIndex;
                  dependency.dstSubpass = subpassIndex + 1;
                  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                  dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                  subpassIndex++;
            }
      }

      // dependencies.insert(dependencies.begin(), firstDependency);

      VkRenderPassCreateInfo renderPassInfo{};
      renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
      std::vector<VkAttachmentDescription> attachments = {
          colorAttachment,
          depthAttachment
      };
      renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
      renderPassInfo.pAttachments = attachments.data();
      renderPassInfo.subpassCount = subpassCount;
      renderPassInfo.pSubpasses = subpasses.data();

      renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
      renderPassInfo.pDependencies = dependencies.data();

      auto res = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
      logger::log_cond_err(res == VK_SUCCESS, "failed to create render pass");

      return NVE_SUCCESS;
}
void Renderer::recreate_render_pass()
{
      if (m_lastGeometryHandlerSubpassCount < geometry_handler_subpass_count())
      {
            create_render_pass();
            create_framebuffers();
            set_geometry_handler_subpasses();
            create_geometry_pipelines();
            update_geometry_handler_framebuffers();
            m_lastGeometryHandlerSubpassCount = geometry_handler_subpass_count();
      }
}
NVE_RESULT Renderer::create_framebuffers()
{
      for (auto& framebuffer : m_swapchainFramebuffers)
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
      m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

      for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
            std::vector<VkImageView> attachments = {
                m_swapchainImageViews[i],
                m_depthImages[i].m_imageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapchainExtent.width;
            framebufferInfo.height = m_swapchainExtent.height;
            framebufferInfo.layers = 1;

            auto res = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]);
            logger::log_cond_err(res == VK_SUCCESS, "failed to create framebuffer no " + i);
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

      logger::log_cond_err(res == VK_SUCCESS, "failed to create command pool");

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_commandbuffers()
{
      m_commandBuffers.resize(m_swapchainFramebuffers.size());

      VkCommandBufferAllocateInfo allocInfo{};
      allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
      allocInfo.pNext = nullptr;
      allocInfo.commandPool = m_commandPool;
      allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
      allocInfo.commandBufferCount = m_swapchainFramebuffers.size();

      auto res = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
      logger::log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer");

      return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_sync_objects()
{
      m_imageAvailableSemaphores.resize(m_swapchainFramebuffers.size());
      m_renderFinishedSemaphores.resize(m_swapchainFramebuffers.size());
      m_inFlightFences.resize(m_swapchainFramebuffers.size());

      VkSemaphoreCreateInfo semCI = {};
      semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

      VkFenceCreateInfo fenceCI = {};
      fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;


      for (size_t frame = 0; frame < m_swapchainFramebuffers.size(); frame++)
      {
            auto res = VK_SUCCESS;

            res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_imageAvailableSemaphores[frame]);
            logger::log_cond_err(res == VK_SUCCESS, "failed to create imageAvailable semaphore");

            res = vkCreateSemaphore(m_device, &semCI, nullptr, &m_renderFinishedSemaphores[frame]);
            logger::log_cond_err(res == VK_SUCCESS, "failed to create renderFinished semaphore");

            res = vkCreateFence(m_device, &fenceCI, nullptr, &m_inFlightFences[frame]);
            logger::log_cond_err(res == VK_SUCCESS, "failed to create inFlight fence");
      }

      return NVE_SUCCESS;
}
void Renderer::create_depth_images()
{
      m_depthImages.resize(m_swapchainImages.size());
      for (auto& image : m_depthImages)
      {
            image.create(
                  m_device,
                  m_physicalDevice,
                  m_swapchainExtent.width,
                  m_swapchainExtent.height,
                  find_depth_format(m_physicalDevice),
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  VK_IMAGE_ASPECT_DEPTH_BIT
            );
      }
}

void Renderer::create_descriptor_pool()
{
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

      auto res = vkCreateDescriptorPool(m_device, &descPoolCI, nullptr, &m_descriptorPool);
      logger::log_cond_err(res == VK_SUCCESS, "failed to create descriptor pool");
}

void Renderer::initialize_geometry_handlers()
{
      GeometryHandlerVulkanObjects vulkanObjects;
      vulkanObjects.device = m_device;
      vulkanObjects.physicalDevice = m_physicalDevice;
      vulkanObjects.commandPool = m_commandPool;
      vulkanObjects.renderPass = &m_renderPass;
      vulkanObjects.transferQueue = m_transferQueue;
      vulkanObjects.queueFamilyIndex = m_queueFamilyIndices.transferFamily.value();
      vulkanObjects.descriptorPool = m_descriptorPool;

      vulkanObjects.swapchainExtent = m_swapchainExtent;
      // vulkanObjects.framebuffers = std::vector<VkFramebuffer*>(m_swapchainFramebuffers.size());
      // for (size_t i = 0; i < vulkanObjects.framebuffers.size(); i++)
      //     vulkanObjects.framebuffers[i] = &m_swapchainFramebuffers[i];
      vulkanObjects.framebuffers = m_swapchainFramebuffers;
      vulkanObjects.firstSubpass = 0;

      vulkanObjects.pCameraPushConstant = &m_cameraPushConstant;

      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            handler->initialize(vulkanObjects, &m_guiManager);
      }
      set_geometry_handler_subpasses();

      m_ecs.register_system<StaticGeometryHandler>(&m_staticGeometryHandler);
      m_ecs.register_system<DynamicGeometryHandler>(&m_dynamicGeometryHandler);
      m_ecs.register_system<GizmosHandler>(&m_gizmosHandler);
}
void Renderer::set_geometry_handler_subpasses()
{
      uint32_t subpass = 0;
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            handler->set_first_subpass(subpass);
            subpass += handler->subpass_count();
      }
}
void Renderer::update_geometry_handler_framebuffers()
{
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
            handler->update_framebuffers(m_swapchainFramebuffers, m_swapchainExtent);
}
void Renderer::create_geometry_pipelines()
{
      auto handlers = all_geometry_handlers();

      for (GeometryHandler* geometryHandler : handlers)
      {
            std::vector<VkGraphicsPipelineCreateInfo> pipelineCIs;

            geometryHandler->create_pipeline_create_infos(pipelineCIs);
            if (pipelineCIs.empty())
                  continue;

            std::vector<VkPipeline> pipelines(pipelineCIs.size());
            auto res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, pipelineCIs.size(), pipelineCIs.data(), nullptr, pipelines.data());
            logger::log_cond_err(res == VK_SUCCESS, "failed to create geometry handler pipelines");

            geometryHandler->set_pipelines(pipelines);
      }
}
std::vector<GeometryHandler*> Renderer::all_geometry_handlers()
{
      return {
          &m_staticGeometryHandler,
          &m_dynamicGeometryHandler,
          &m_gizmosHandler
      };
}
// TODO staged buffer copy synchronization
void Renderer::wait_for_geometry_handler_buffer_cpies()
{
      std::vector<VkFence> fences;
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
      {
            auto handlerFences = handler->buffer_cpy_fences();
            append_vector(fences, handlerFences);
      }
      vkWaitForFences(m_device, static_cast<uint32_t>(fences.size()), fences.data(), VK_TRUE, UINT32_MAX);
}

void Renderer::destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
      auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
      if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
      }
}

void Renderer::record_main_command_buffer(uint32_t frame)
{
      // -------------------------------------------

      vkResetCommandBuffer(m_commandBuffers[frame], 0);

      // -------------------------------------------

      VkCommandBuffer commandBuffer = m_commandBuffers[frame];

      // -------------------------------------------

      VkCommandBufferBeginInfo commandBufferBI = {};
      commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      commandBufferBI.pNext = nullptr;
      commandBufferBI.flags = 0;
      commandBufferBI.pInheritanceInfo = nullptr;

      {
            auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
            logger::log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording");
      }

      // -------------------------------------------

      VkRenderPassBeginInfo renderPassBI = {};
      renderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      renderPassBI.renderPass = m_renderPass;
      renderPassBI.framebuffer = m_swapchainFramebuffers[frame];
      renderPassBI.renderArea.offset = { 0, 0 };
      renderPassBI.renderArea.extent = m_swapchainExtent;

      std::vector<VkClearValue> clearValues(2);
      clearValues[0].color = { m_config.clearColor.x / 255.f, m_config.clearColor.y / 255.f, m_config.clearColor.z / 255.f, 1.f };
      clearValues[1].depthStencil = { 1.0f, 0 };
      renderPassBI.clearValueCount = static_cast<uint32_t>(clearValues.size());
      renderPassBI.pClearValues = clearValues.data();

      vkCmdBeginRenderPass(commandBuffer, &renderPassBI, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

      // -------------------------------------------

      std::vector<VkCommandBuffer> secondaryCommandBuffers;
      auto geometryHandlers = all_geometry_handlers();
      for (auto geometryHandler : geometryHandlers)
      {
            auto geometryHandlerCommandBuffers = geometryHandler->get_command_buffers(frame);
            append_vector(secondaryCommandBuffers, geometryHandlerCommandBuffers);
      }

      uint32_t subpassCount = secondaryCommandBuffers.size();
      for (uint32_t subpass = 0; subpass < subpassCount; subpass++)
      {
            if (subpass < secondaryCommandBuffers.size())
                  vkCmdExecuteCommands(commandBuffer, 1, &secondaryCommandBuffers[subpass]);
            if (subpass != subpassCount - 1)
                  vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
      }

      // -------------------------------------------

      vkCmdEndRenderPass(commandBuffer);

      {
            auto res = vkEndCommandBuffer(commandBuffer);
            logger::log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording: " + std::string(string_VkResult(res)));
      }

      // -------------------------------------------
}
NVE_RESULT Renderer::submit_command_buffers(
      std::vector<VkCommandBuffer> commandBuffers,
      std::vector<VkSemaphore> waitSems,
      std::vector<VkPipelineStageFlags> waitStages,
      std::vector<VkSemaphore> signalSems
)
{
      VkSubmitInfo submitInfo{};
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSems.size());
      submitInfo.pWaitSemaphores = waitSems.data();
      submitInfo.pWaitDstStageMask = waitStages.data();

      submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
      submitInfo.pCommandBuffers = commandBuffers.data();

      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSems.size());
      submitInfo.pSignalSemaphores = signalSems.data();

      auto res = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_frame]);
      logger::log_cond(res != VK_SUCCESS, "failed to submit command buffers to graphics queue: " + std::string(string_VkResult(res)));

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
      if (m_firstFrame)
            first_frame();

      //auto renderStart = std::chrono::high_resolution_clock::now();
      float renderTime = 0;
      m_profiler.begin_label("draw_frame");
      PROFILE_START("await fences");
      // Wait for the previous frame to finish
      if (!m_acquireImageTimeout)
      {
            vkWaitForFences(m_device, 1, &m_inFlightFences[m_frame], VK_TRUE, UINT64_MAX);
            vkResetFences(m_device, 1, &m_inFlightFences[m_frame]);
      }
      renderTime += PROFILE_END("await fences");

      PROFILE_START("render pass recreation");
      // recreate render pass if needed
      recreate_render_pass();
      renderTime += PROFILE_END("render pass recreation");

      PROFILE_START("acquire image");
      // Acquire an image from the swap chain
      uint32_t imageIndex;
      {
            auto res = vkAcquireNextImageKHR(m_device, m_swapchain, 1, m_imageAvailableSemaphores[m_frame], VK_NULL_HANDLE, &imageIndex);
            if (res == VK_TIMEOUT)
            {
                  m_acquireImageTimeout = true;
                  return NVE_SUCCESS;
            }
            else
            {
                  m_acquireImageTimeout = false;
            }
      }
      renderTime += PROFILE_END("acquire image");

      // update push constant
      m_cameraPushConstant.projView = m_activeCamera->projection_matrix() * m_activeCamera->view_matrix();
      m_cameraPushConstant.camPos = m_activeCamera->m_position;

      // collect semaphores
      // this has to be done before the model handler update because the geometry handlers otherwise doesn't update its buffers
      std::vector<VkSemaphore> waitSemaphores = { m_imageAvailableSemaphores[m_frame] };

      // record command buffers
      PROFILE_START("record cmd buffers");
      auto geometryHandlers = all_geometry_handlers();
      for (auto geometryHandler : geometryHandlers)
      {
            // TODO staged buffer copy synchronization
            //auto sems = geometryHandler->buffer_cpy_semaphores();
            //for (auto sem : sems)
            //    if (sem != VK_NULL_HANDLE)
            //        waitSemaphores.push_back(sem);

            m_threadPool.doJob(std::bind(&Renderer::genCmdBuf, this, geometryHandler));
      }
      m_threadPool.wait_for_finish();
      renderTime += PROFILE_END("record cmd buffers");

      PROFILE_START("record main cmd buf");
      record_main_command_buffer(m_frame);
      renderTime += PROFILE_END("record main cmd buf");
      PROFILE_START("gui draw");
      if (!m_imguiDraw)
            gui_begin();

      imgui_draw(imageIndex);
      m_imguiDraw = false;
      renderTime += PROFILE_END("gui draw");

      std::vector<VkSemaphore> signalSemaphores = { m_renderFinishedSemaphores[m_frame] };
      std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

      // TODO staged buffer copy synchronization
      //while (waitSemaphores.size() > waitStages.size())
      //    waitStages.push_back(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);

      // wait_for_geometry_handler_buffer_cpies();

      PROFILE_START("submit cmd buf");
      // Submit the recorded command buffers
      std::vector<VkCommandBuffer> commandBuffers = { m_commandBuffers[m_frame], m_imgui_commandBuffers[m_frame] };
      submit_command_buffers(commandBuffers, waitSemaphores, waitStages, signalSemaphores);
      renderTime += PROFILE_END("submit cmd buf");

      PROFILE_START("present image");
      // Present the swap chain image
      present_swapchain_image(m_swapchain, imageIndex, signalSemaphores);
      renderTime += PROFILE_END("present image");
#ifdef RENDER_PROFILER
      m_profiler.out_buf() << "total render time: " << renderTime << " seconds\n";
#endif
      m_profiler.end_label();

      m_frame = (m_frame + 1) % m_swapchainFramebuffers.size();

      //auto renderEnd = std::chrono::high_resolution_clock::now();
      //std::cout << "total render time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(renderEnd - renderStart).count() / NANOSECONDS_PER_SECOND;

      return NVE_SUCCESS;
}
void Renderer::first_frame()
{
      recreate_render_pass();
      create_geometry_pipelines();

      m_firstFrame = false;
}

void Renderer::clean_up()
{
      if (!m_initialized)
            return;
      m_initialized = false;

      vkDeviceWaitIdle(m_device);

      imgui_cleanup();

      geometry_handler_cleanup();

      vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

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

      for (auto& image : m_depthImages)
            image.destroy();
      for (auto imageView : m_swapchainImageViews)
            vkDestroyImageView(m_device, imageView, nullptr);

      destroy_all_corresponding_buffers(m_device);

      vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
      vkDestroyDevice(m_device, nullptr);
      vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

      // if (m_config.enableValidationLayers)
      //     destroy_debug_messenger(m_instance, m_debugMessenger, nullptr);

      vkDestroyInstance(m_instance, nullptr);

      glfwDestroyWindow(m_window);
      glfwTerminate();
}
void Renderer::geometry_handler_cleanup()
{
      auto handlers = all_geometry_handlers();
      for (auto handler : handlers)
            handler->cleanup();
}

uint32_t Renderer::geometry_handler_subpass_count()
{
      auto handlers = all_geometry_handlers();
      uint32_t subpassCount = 0;
      for (auto handler : handlers)
            subpassCount += handler->subpass_count();
      return subpassCount;
}

void Renderer::init_default_camera()
{
      set_active_camera(&m_defaultCamera);
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

void Renderer::genCmdBuf(GeometryHandler* geometryHandler)
{
      geometryHandler->record_command_buffers(m_frame);
};
void Renderer::await_last_frame_render()
{
      vkWaitForFences(m_device, 1, &m_inFlightFences[(m_frame + m_inFlightFences.size() - 1) % m_inFlightFences.size()], VK_TRUE, UINT64_MAX);
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
      m_imguiDraw = false;

      logger::log("imgui initialized");

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
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui render pass");

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
            logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui framebuffer no " + i);
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
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui descriptor pool");

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
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui command pool");

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
      logger::log_cond_err(res == VK_SUCCESS, "failed to create imgui command buffers");

      return NVE_SUCCESS;
}

void Renderer::gui_begin()
{
      if (!m_imguiDraw && !m_acquireImageTimeout)
      {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            m_imguiDraw = true;
      }
}
void Renderer::draw_engine_gui(std::function<void(void)> guiDraw)
{
      if (m_acquireImageTimeout)
            return;
      gui_begin();

      m_guiManager.activate();
      m_guiManager.draw_entity_info();
      m_guiManager.draw_system_info();

      guiDraw();
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
            logger::log_cond_err(res == VK_SUCCESS, "failed to begin imgui command buffer on image index " + imageIndex);
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
      logger::log_cond_err(res == VK_SUCCESS, "failed to end imgui command buffer no " + imageIndex);
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
      attributeDescriptions[1].offset = offsetof(Vertex, normal);

      attributeDescriptions[2].binding = 0;
      attributeDescriptions[2].location = 2;
      attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
      attributeDescriptions[2].offset = offsetof(Vertex, color);

      attributeDescriptions[3].binding = 0;
      attributeDescriptions[3].location = 3;
      attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
      attributeDescriptions[3].offset = offsetof(Vertex, uv);

      attributeDescriptions[4].binding = 0;
      attributeDescriptions[4].location = 4;
      attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
      attributeDescriptions[4].offset = offsetof(Vertex, material);

      return attributeDescriptions;
}

bool Vertex::operator== (const Vertex& other) const
{
      return pos == other.pos && normal == other.normal && color == other.color && uv == other.uv;
}

bool operator<(const Vector3& l, const Vector3& r)
{
      return
            l.x < r.x &&
            l.y < r.y &&
            l.z < r.z
            ;
}
bool operator<(const Vector2& l, const Vector2& r)
{
      return
            l.x < r.x &&
            l.y < r.y
            ;
}

bool Vertex::operator<(const Vertex& other)
{
      return
            pos < other.pos &&
            normal < other.normal &&
            color < other.color &&
            uv < other.uv &&
            material < other.material
            ;
}

// ---------------------------------------
// CAMERA
// ---------------------------------------
Camera::Camera() :
      m_position(0), m_rotation(0), m_fov(90), m_nearPlane(0.01f), m_farPlane(1000.f), m_extent(1080, 1920), renderer(nullptr), m_orthographic{ false }
{}
glm::mat4 Camera::view_matrix()
{
      if (!m_orthographic)
            return glm::lookAt(m_position, m_position + glm::rotate(glm::qua(glm::radians(m_rotation)), VECTOR_FORWARD), VECTOR_DOWN);

      float zoom = 1.f / math::max(0.001f, m_position.z + 1);
      float ratio = m_extent.y / m_extent.x;
      return glm::transpose(glm::mat4x4(
            zoom * ratio, 0, 0, m_position.x * ratio,
            0, zoom, 0, m_position.y,
            0, 0, 1, 0,
            0, 0, 0, 1
      ));
}
glm::mat4 Camera::projection_matrix()
{
      if (!m_orthographic)
            return glm::perspective(glm::radians(m_fov), m_extent.x / m_extent.y, m_nearPlane, m_farPlane);
      return glm::transpose(glm::mat4x4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 1
      ));
}

#undef PROFILE_START
#undef PROFILE_END
#undef PROFILE_LABEL
#undef PROFILE_LABEL_END
