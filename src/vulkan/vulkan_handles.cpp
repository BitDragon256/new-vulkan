#include "vulkan/vulkan_handles.h"

#include <set>

#include <vulkan/vk_enum_string_helper.h>

#include "vulkan/vulkan_helpers.h"
#include "logger.h"

#define VK_CHECK_ERROR(res) { vk_check_error(res, __FILE__, __LINE__); }
inline void vk_check_error(VkResult result, const char* file, int line)
{
      if (result != VK_SUCCESS)
            fprintf(stderr, "vulkan error in file %s, line %d: %s\n", file, line, string_VkResult(result));
}

inline void print_not_initialized_error()
{
      printf("ERROR: Vulkan Handle is not initialized");
}

namespace vk
{

      // --------------------------------
      // VULKAN HANDLE
      // --------------------------------

      VulkanHandle::VulkanHandle() :
            m_created{ false }, m_initialized{ false }
      {}
      void VulkanHandle::on_update()
      {
            if (!m_initialized)
                  print_not_initialized_error();

            if (m_created)
                  destroy();
            create();
            m_created = true;
      }
      void VulkanHandle::initialize()
      {
            m_initialized = true;
      }

      // --------------------------------
      // VULKAN INSTANCE
      // --------------------------------

      void Instance::initialize(std::string applicationName, uint32_t applicationVersion, std::string engineName, bool enableValidationLayers)
      {
            m_applicationName = applicationName;
            m_applicationVersion = applicationVersion;
            m_engineName = engineName;
            m_enableValidationLayers = enableValidationLayers;

            VulkanHandle::initialize();
      }
      void Instance::create()
      {
            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = m_applicationName.c_str();
            appInfo.applicationVersion = m_applicationVersion;
            appInfo.pEngineName = m_engineName.c_str();
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_3;
            appInfo.pNext = nullptr;

            VkInstanceCreateInfo instanceCI = {};
            instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceCI.pApplicationInfo = &appInfo;

            if (m_enableValidationLayers)
                  m_instanceLayers.push_back("VK_LAYER_KHRONOS_validation");

            instanceCI.enabledLayerCount = static_cast<uint32_t>(m_instanceLayers.size());
            instanceCI.ppEnabledLayerNames = m_instanceLayers.data();

            //VkDebugUtilsMessengerCreateInfoEXT debugMessengerCI = {};
            //populate_debug_messenger_create_info(debugMessengerCI);
            instanceCI.pNext = nullptr;//&debugMessengerCI;

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
            //std::vector<const char*> extensions{ "VK_KHR_surface" };

            if (m_enableValidationLayers)
                  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            instanceCI.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instanceCI.ppEnabledExtensionNames = extensions.data();
            instanceCI.flags = 0;

            auto res = vkCreateInstance(&instanceCI, nullptr, &m_instance);
            VK_CHECK_ERROR(res)
      }
      void Instance::destroy()
      {
            vkDestroyInstance(m_instance, nullptr);
      }
      Instance::operator VkInstance()
      {
            return m_instance;
      }

      // --------------------------------
      // PHYSICAL DEVICE
      // --------------------------------

      void PhysicalDevice::initialize(REF(Instance) instance, REF(Surface) surface)
      {
            add_dependency(instance);
            add_dependency(surface);
      }
      void PhysicalDevice::create()
      {
            auto instance = get_dependency<Instance>();
            auto surface = get_dependency<Surface>();

            // get all available physical devices
            uint32_t deviceCount;
            vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);
            std::vector<VkPhysicalDevice> availableDevices(deviceCount);
            vkEnumeratePhysicalDevices(*instance, &deviceCount, availableDevices.data());

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
                  if (is_device_suitable(device, *surface))
                  {
                        m_physicalDevice = device;
                        break;
                  }
            }
            logger::log_cond_err(m_physicalDevice != VK_NULL_HANDLE, "no acceptable physical device found");

#endif
      }
      void PhysicalDevice::destroy()
      {

      }
      PhysicalDevice::operator VkPhysicalDevice()
      {
            return m_physicalDevice;
      }

      // --------------------------------
      // DEVICE
      // --------------------------------

      void Device::create()
      {
            auto physicalDevice = get_dependency<PhysicalDevice>();
            auto surface = get_dependency<Surface>();
            
            m_queueFamilyIndices = find_queue_families(*physicalDevice, *surface);

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
            deviceCI.flags = 0;

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

            VK_CHECK_ERROR(vkCreateDevice(*physicalDevice, &deviceCI, nullptr, &m_device));

            vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue);
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue);
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue);
      }
      void Device::destroy()
      {
            vkDestroyDevice(m_device, nullptr);
      }

      // --------------------------------
      // SURFACE
      // --------------------------------

      void Surface::create()
      {
            auto instance = get_dependency<Instance>();
            auto window = get_dependency<Window>();
            
            auto res = glfwCreateWindowSurface(*instance, *window, nullptr, &m_surface);
            VK_CHECK_ERROR(res)
      }
      void Surface::destroy()
      {
            auto instance = get_dependency<Instance>();

            vkDestroySurfaceKHR(*instance, m_surface, nullptr);
      }

      // --------------------------------
      // SWAPCHAIN
      // --------------------------------

      VkImageViewCreateInfo Swapchain::swapchain_image_view_create_info(uint32_t index)
      {
            VkImageViewCreateInfo imageViewCI = {};

            imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCI.image = m_images[index];
            imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewCI.format = m_imageFormat;
            imageViewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewCI.subresourceRange.baseMipLevel = 0;
            imageViewCI.subresourceRange.levelCount = 1;
            imageViewCI.subresourceRange.baseArrayLayer = 0;
            imageViewCI.subresourceRange.layerCount = 1;

            return imageViewCI;
      }

      void Swapchain::create()
      {
            auto physicalDevice = get_dependency<PhysicalDevice>();
            auto surface = get_dependency<Surface>();
            auto window = get_dependency<Window>();
            auto device = get_dependency<Device>();

            SwapChainSupportDetails swapChainSupport = query_swap_chain_support(*physicalDevice, *surface);

            VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
            VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
            VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities, *window);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
                  imageCount = swapChainSupport.capabilities.maxImageCount;
            }
            VkSwapchainCreateInfoKHR swapchainCI = {};
            swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainCI.surface = *surface;
            swapchainCI.minImageCount = imageCount;
            swapchainCI.imageFormat = surfaceFormat.format;
            swapchainCI.imageColorSpace = surfaceFormat.colorSpace;
            swapchainCI.imageExtent = extent;
            swapchainCI.imageArrayLayers = 1;
            swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            uint32_t queueFamilyIndices[] = { device->m_queueFamilyIndices.graphicsFamily.value(), device->m_queueFamilyIndices.presentationFamily.value() };

            if (device->m_queueFamilyIndices.graphicsFamily != device->m_queueFamilyIndices.presentationFamily) {
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

            VK_CHECK_ERROR(vkCreateSwapchainKHR(*device, &swapchainCI, nullptr, &m_swapchain));

            vkGetSwapchainImagesKHR(*device, m_swapchain, &imageCount, nullptr);
            std::vector<VkImage> images(imageCount);
            vkGetSwapchainImagesKHR(*device, m_swapchain, &imageCount, images.data());

            m_imageFormat = surfaceFormat.format;
            m_extent = extent;

            m_images.resize(imageCount);
            for (size_t i = 0; i < images.size(); i++)
                  m_images[i].create(device, images[i], swapchain_image_view_create_info(i));
      }
      void Swapchain::destroy()
      {
            auto device = get_dependency<Device>();
            for (auto& image : m_images)
                  image.destroy();
            vkDestroySwapchainKHR(*device, m_swapchain, nullptr);
      }

      // --------------------------------
      // IMAGE
      // --------------------------------

      void Image::create()
      {
            auto createInfo = get_dependency<ImageCreateInfo>();
            auto device = get_dependency<Image>();
            create_image_view(createInfo->imageView);
            create_image(createInfo->image);
      }
      void Image::destroy()
      {

      }
      void Image::create(Reference<Device> device, VkImage image, const VkImageViewCreateInfo& imageViewCI)
      {
            m_device = device;
            m_image = image;
            create_image_view(imageViewCI);
      }
      void Image::create(Reference<Device> device, const VkImageCreateInfo& imageCI, const VkImageViewCreateInfo& imageViewCI)
      {
            m_device = device;

            create_image(imageCI);
            create_image_view(imageViewCI);
      }
      void Image::destroy()
      {
            vkDestroyImageView(*m_device, m_imageView, nullptr);
            vkDestroyImage(*m_device, m_image, nullptr);
      }
      void Image::create_image(const VkImageCreateInfo& imageCI)
      {
            m_extent = imageCI.extent;
            auto res = vkCreateImage(*m_device, &imageCI, nullptr, &m_image);
            VK_CHECK_ERROR(res)
      }
      void Image::create_image_view(const VkImageViewCreateInfo& imageViewCI)
      {
            auto res = vkCreateImageView(*m_device, &imageViewCI, nullptr, &m_imageView);
            VK_CHECK_ERROR(res)
      }

      // --------------------------------
      // RENDER PASS
      // --------------------------------

      void RenderPass::create()
      {
            auto device = get_dependency<Device>();
            auto physicalDevice = get_dependency<PhysicalDevice>();
            auto swapchain = get_dependency<Swapchain>();

            // color attachment

            VkAttachmentDescription colorAttachment = {};
            colorAttachment.format = swapchain->m_imageFormat;
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
            depthAttachment.format = find_depth_format(*physicalDevice);
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

            auto res = vkCreateRenderPass(*device, &renderPassInfo, nullptr, &m_renderPass);
            VK_CHECK_ERROR(res)
      }
      void RenderPass::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyRenderPass(*device, m_renderPass, nullptr);
      }

      // --------------------------------
      // FRAMEBUFFER
      // --------------------------------

      void Framebuffer::create()
      {
            auto renderPass = get_dependency<RenderPass>();
            auto device = get_dependency<Device>();
            auto images = get_dependencies<Image>();

            std::vector<VkImageView> attachments(images.size());
            for (size_t i = 0; i < images.size(); i++)
                  attachments[i] = images[i]->m_imageView;

            VkFramebufferCreateInfo framebufferCI;
            framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCI.renderPass = *renderPass;
            framebufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCI.pAttachments = attachments.data();
            framebufferCI.width = images[0]->m_extent.width;
            framebufferCI.height = images[0]->m_extent.height;
            framebufferCI.layers = 1;

            auto res = vkCreateFramebuffer(*device, &framebufferCI, nullptr, &m_framebuffer);
            VK_CHECK_ERROR(res)
      }
      void Framebuffer::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyFramebuffer(*device, m_framebuffer, nullptr);
      }

      // --------------------------------
      // COMMAND POOL
      // --------------------------------

      void CommandPool::create()
      {
            auto device = get_dependency<Device>();
            auto createInfos = get_dependency<CommandPoolCreateInfo>();

            VkCommandPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            createInfo.flags = 0;
            createInfo.pNext = nullptr;
            createInfo.queueFamilyIndex = createInfos->queueFamilyIndex;

            auto res = vkCreateCommandPool(*device, &createInfo, nullptr, &m_commandPool);
            VK_CHECK_ERROR(res)
      }
      void CommandPool::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyCommandPool(*device, m_commandPool, nullptr);
      }

      // --------------------------------
      // COMMAND BUFFER
      // --------------------------------

      void CommandBuffer::create()
      {
            auto device = get_dependency<Device>();
            auto commandPool = get_dependency<CommandPool>();

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.commandPool = *commandPool;

            auto res = vkAllocateCommandBuffers(*device, &allocInfo, nullptr);
            VK_CHECK_ERROR(res)
      }

      // --------------------------------
      // SEMAPHORE
      // --------------------------------

      void Semaphore::create()
      {
            auto device = get_dependency<Device>();

            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;

            auto res = vkCreateSemaphore(*device, &createInfo, nullptr, &m_semaphore);
            VK_CHECK_ERROR(res)
      }
      void Semaphore::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroySemaphore(*device, m_semaphore, nullptr);
      }

      // --------------------------------
      // FENCE
      // --------------------------------

      void Fence::create()
      {
            auto device = get_dependency<Device>();

            VkFenceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;

            auto res = vkCreateFence(*device, &createInfo, nullptr, &m_fence);
            VK_CHECK_ERROR(res)
      }
      void Fence::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyFence(*device, m_fence, nullptr);
      }

      // --------------------------------
      // DEBUG UTILS MESSENGER
      // --------------------------------

      // --------------------------------
      // DESCRIPTOR POOL
      // --------------------------------

      void DescriptorPool::create()
      {
            auto device = get_dependency<Device>();

            // 11
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
            const uint32_t poolSizeCount = 11;

            VkDescriptorPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            createInfo.maxSets = 1000 * poolSizeCount;
            createInfo.poolSizeCount = poolSizeCount;
            createInfo.pPoolSizes = poolSizes;

            auto res = vkCreateDescriptorPool(*device, &createInfo, nullptr, &m_descriptorPool);
            VK_CHECK_ERROR(res)
      }
      void DescriptorPool::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyDescriptorPool(*device, m_descriptorPool, nullptr);
      }

}; // namespace vk