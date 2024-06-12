#include "vulkan/vulkan_handles.h"

#include <set>

#include <vulkan/vk_enum_string_helper.h>

#include "vulkan/vulkan_helpers.h"
#include "logger.h"

#define VKH_LOG(M) logger::log(M);

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
      // HELPER FUNCTIONS
      // --------------------------------

      VkImageCreateInfo fill_image_create_info(
            uint32_t width, uint32_t height, uint32_t depth,
            VkFormat format,
            VkImageUsageFlags usageFlags,
            VkImageLayout initialLayout,
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
            uint32_t mipLevels = 1,
            VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            VkImageType imageType = VK_IMAGE_TYPE_2D,
            uint32_t arrayLayers = 1
      )
      {
            VkImageCreateInfo imageCI = {};
            imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageCI.pNext = nullptr;
            imageCI.flags = 0;

            imageCI.extent.width = width;
            imageCI.extent.height = height;
            imageCI.extent.depth = depth;

            imageCI.format = format;
            imageCI.tiling = tiling;
            imageCI.usage = usageFlags;
            imageCI.initialLayout = initialLayout;

            imageCI.imageType = imageType;
            imageCI.mipLevels = mipLevels;
            imageCI.arrayLayers = arrayLayers;
            imageCI.samples = sampleCount;
            imageCI.sharingMode = sharingMode;

            return imageCI;
      }

      VkImageViewCreateInfo fill_image_view_create_info(
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectMask,
            uint32_t baseMipLevel = 0, uint32_t mipLevelCount = 1,
            uint32_t baseArrayLayer = 0, uint32_t arrayLayerCount = 1,
            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
            VkComponentMapping componentMapping = {
                  VK_COMPONENT_SWIZZLE_IDENTITY,
                  VK_COMPONENT_SWIZZLE_IDENTITY,
                  VK_COMPONENT_SWIZZLE_IDENTITY
            }
      )
      {
            VkImageViewCreateInfo imageViewCI = {};
            imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewCI.pNext = nullptr;
            imageViewCI.flags = 0;
            
            imageViewCI.image = image;
            imageViewCI.viewType = viewType;
            imageViewCI.format = format;
            imageViewCI.components = componentMapping;
            imageViewCI.subresourceRange = {
                  aspectMask,
                  baseMipLevel, mipLevelCount,
                  baseArrayLayer, arrayLayerCount
            };

            return imageViewCI;
      }
      VkMemoryAllocateInfo fill_memory_allocation_info(
            uint32_t size,
            uint32_t memoryType
      )
      {
            VkMemoryAllocateInfo memoryAI = {};
            memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAI.pNext = nullptr;
            memoryAI.allocationSize = size;
            memoryAI.memoryTypeIndex = memoryType;

            return memoryAI;
      }

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
                  unresolve();
            create();
            m_created = true;
      }
      void VulkanHandle::on_unresolve()
      {
            if (m_created)
                  destroy();

            m_created = false;
      }
      void VulkanHandle::initialize()
      {
            NVE_ASSERT(m_created == false)

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

            VKH_LOG("Instance created")
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

            VulkanHandle::initialize();
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

            VKH_LOG("Physical Device created")
      }
      void PhysicalDevice::destroy()
      {
            VKH_LOG("Physical Device destroyed")
      }
      PhysicalDevice::operator VkPhysicalDevice()
      {
            return m_physicalDevice;
      }

      // --------------------------------
      // QUEUE
      // --------------------------------

      void Queue::initialize(VkQueue queue)
      {
            m_queue = queue;
            VulkanHandle::initialize();
      }
      void Queue::create() {}
      void Queue::destroy() {}
      VkQueue& Queue::get()
      {
            return m_queue;
      }
      Queue::operator VkQueue()
      {
            return m_queue;
      }

      void Queue::submit(const VkSubmitInfo& submitInfo, REF(Fence) fence)
      {
            submit(std::vector<VkSubmitInfo>{ submitInfo }, fence);
      }
      void Queue::submit(const std::vector<VkSubmitInfo>& submits)
      {
            vkQueueSubmit(m_queue, static_cast<uint32_t>(submits.size()), submits.data(), VK_NULL_HANDLE);
      }
      void Queue::submit(const std::vector<VkSubmitInfo>& submits, REF(Fence) fence)
      {
            vkQueueSubmit(m_queue, static_cast<uint32_t>(submits.size()), submits.data(), *fence);
      }

      void Queue::present(const VkPresentInfoKHR& presentInfo)
      {
            vkQueuePresentKHR(m_queue, &presentInfo);
      }

      void Queue::submit_command_buffers(
            std::vector<VkCommandBuffer>& commandBuffers,
            std::vector<REF(Semaphore)>& waitSemaphores,
            std::vector<VkPipelineStageFlags>& waitStages,
            std::vector<REF(Semaphore)>& signalSemaphores,
            REF(Fence) fence
      )
      {
            std::vector<VkSemaphore> waitVkSemaphores(waitSemaphores.size());
            for (size_t i = 0; i < waitSemaphores.size(); i++) waitVkSemaphores[i] = *waitSemaphores[i];

            std::vector<VkSemaphore> signalVkSemaphores(signalSemaphores.size());
            for (size_t i = 0; i < signalSemaphores.size(); i++) signalVkSemaphores[i] = *signalSemaphores[i];

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitVkSemaphores.size());
            submitInfo.pWaitSemaphores = waitVkSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();

            submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
            submitInfo.pCommandBuffers = commandBuffers.data();

            submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalVkSemaphores.size());
            submitInfo.pSignalSemaphores = signalVkSemaphores.data();

            submit(submitInfo, fence);
      }

      // --------------------------------
      // DEVICE
      // --------------------------------

      void Device::initialize(REF(PhysicalDevice) physicalDevice, REF(Surface) surface)
      {
            add_dependency(physicalDevice);
            add_dependency(surface);

            VulkanHandle::initialize();
      }
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

            vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue.get());
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue.get());
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.transferFamily.value(), 0, &m_transferQueue.get());
            vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily.value(), 0, &m_computeQueue.get());

            VKH_LOG("Device created")
      }
      void Device::destroy()
      {
            vkDestroyDevice(m_device, nullptr);

            VKH_LOG("Device destroyed")
      }
      Device::operator VkDevice()
      {
            return m_device;
      }

      uint32_t Device::graphics_queue_family() { return m_queueFamilyIndices.graphicsFamily.value(); }
      uint32_t Device::presentation_queue_family() { return m_queueFamilyIndices.presentationFamily.value(); }
      uint32_t Device::transfer_queue_family() { return m_queueFamilyIndices.transferFamily.value(); }
      uint32_t Device::compute_queue_family() { return m_queueFamilyIndices.computeFamily.value(); }

      // --------------------------------
      // WINDOW
      // --------------------------------

      void Window::initialize(int width, int height, std::string title)
      {
            m_width = width;
            m_height = height;
            m_title = title;

            VulkanHandle::initialize();
      }
      void Window::create()
      {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), NULL, NULL);

            VKH_LOG("Window created")
      }
      void Window::destroy()
      {
            glfwDestroyWindow(m_window);

            VKH_LOG("Window destroyed")
      }
      Window::operator GLFWwindow* ()
      {
            return m_window;
      }

      // --------------------------------
      // SURFACE
      // --------------------------------

      void Surface::initialize(REF(Instance) instance, REF(Window) window)
      {
            add_dependency(instance);
            add_dependency(window);

            VulkanHandle::initialize();
      }
      void Surface::create()
      {
            auto instance = get_dependency<Instance>();
            auto window = get_dependency<Window>();
            
            auto res = glfwCreateWindowSurface(*instance, *window, nullptr, &m_surface);
            VK_CHECK_ERROR(res)

            VKH_LOG("Surface created")
      }
      void Surface::destroy()
      {
            auto instance = get_dependency<Instance>();

            vkDestroySurfaceKHR(*instance, m_surface, nullptr);

            VKH_LOG("Surface destroyed")
      }
      Surface::operator VkSurfaceKHR()
      {
            return m_surface;
      }

      // --------------------------------
      // SWAPCHAIN
      // --------------------------------

      void Swapchain::initialize(
            REF(Device) device,
            REF(PhysicalDevice) physicalDevice,
            REF(Window) window,
            REF(Surface) surface,
            REF(RenderPass) renderPass
      )
      {
            add_dependency(device);
            add_dependency(physicalDevice);
            add_dependency(window);
            add_dependency(surface);

            m_frameObectIndex = 0;
            m_lastFrameObjectIndex = 0;

            m_renderPass = renderPass;

            VulkanHandle::initialize();
      }
      void Swapchain::create()
      {
            auto surface = get_dependency<Surface>();
            auto window = get_dependency<Window>();
            auto physicalDevice = get_dependency<PhysicalDevice>();
            auto device = get_dependency<Device>();

            SwapChainSupportDetails swapChainSupport = query_swap_chain_support(*physicalDevice, *surface);

            VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
            VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
            VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities, *window);

            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
            {
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

            uint32_t graphicsQueueFamily = device->graphics_queue_family();
            uint32_t presentationQueueFamily = device->presentation_queue_family();
            uint32_t queueFamilyIndices[] = { graphicsQueueFamily, presentationQueueFamily };

            if (graphicsQueueFamily != presentationQueueFamily)
            {
                  swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                  swapchainCI.queueFamilyIndexCount = 2;
                  swapchainCI.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
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

            m_images.resize(imageCount); m_depthImages.resize(imageCount);
            for (size_t i = 0; i < images.size(); i++)
            {
                  m_images[i].initialize(
                        device,
                        images[i],
                        { m_extent.width, m_extent.height, 1 },
                        m_imageFormat,
                        VK_IMAGE_ASPECT_COLOR_BIT
                  );
                  m_images[i].create();

                  m_depthImages[i].initialize(
                        device,
                        physicalDevice,
                        m_extent.width, m_extent.height,
                        find_depth_format(*physicalDevice),
                        VK_IMAGE_ASPECT_DEPTH_BIT,
                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        VK_IMAGE_LAYOUT_UNDEFINED
                  );
                  m_depthImages[i].add_dependency<Swapchain>(this);
            }

            m_framebuffers.resize(imageCount);
            for (size_t i = 0; i < m_framebuffers.size(); i++)
            {
                  m_framebuffers[i].initialize(
                        device,
                        m_renderPass,
                        { &m_images[i], &m_depthImages[i] }
                  );
            }

            VKH_LOG("Swapchain created")
      }
      void Swapchain::destroy()
      {
            auto device = get_dependency<Device>();
            for (auto& image : m_images)
                  image.destroy();
            vkDestroySwapchainKHR(*device, m_swapchain, nullptr);

            VKH_LOG("Swapchain destroyed")
      }

      Swapchain::operator VkSwapchainKHR()
      {
            return m_swapchain;
      }

      uint32_t Swapchain::size()
      {
            return m_images.size();
      }

      VkResult Swapchain::next_image()
      {
            auto device = get_dependency<Device>();

            return vkAcquireNextImageKHR(
                  *device,
                  m_swapchain,
                  1,
                  m_imageAvailableSemaphores[m_frameObectIndex],
                  VK_NULL_HANDLE,
                  &m_currentImageIndex
            );
      }
      void Swapchain::next_frame()
      {
            m_lastFrameObjectIndex = m_frameObectIndex;

            m_frameObectIndex = (m_frameObectIndex + 1) % size();
      }
      void Swapchain::present_current_image(std::vector<REF(Semaphore)>& semaphores)
      {
            auto device = get_dependency<Device>();
            REF(Queue) presentationQueue = &device->m_presentationQueue;

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());

            std::vector<VkSemaphore> vkSems(semaphores.size());
            for (size_t i = 0; i < semaphores.size(); i++)
                  vkSems[i] = *(semaphores[i]);
            presentInfo.pWaitSemaphores = vkSems.data();

            VkSwapchainKHR swapChains[] = { m_swapchain };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &m_currentImageIndex;

            presentationQueue->present(presentInfo);
      }

      REF(Semaphore) Swapchain::current_image_available_semaphore()
      {
            return &m_imageAvailableSemaphores[m_frameObectIndex];
      }

      // --------------------------------
      // IMAGE
      // --------------------------------

      void Image::base_initialize(REF(Device) device)
      {
            add_dependency(device);
            m_externallyCreated = false;
            m_memoryAllocated = false;

            VulkanHandle::initialize();
      }
      void Image::initialize(
            REF(Device) device,
            VkImage image,
            VkExtent3D extent,
            VkFormat format,
            VkImageAspectFlags aspect
      )
      {
            base_initialize(device);

            m_imageViewCI = fill_image_view_create_info(
                  image,
                  format,
                  aspect
            );
            m_image = image;
            m_extent = extent;
            m_externallyCreated = true;
      }
      void Image::initialize(
            REF(Device) device,
            REF(PhysicalDevice) physicalDevice,
            const VkImageCreateInfo& imageCI,
            const VkImageViewCreateInfo& imageViewCI
      )
      {
            base_initialize(device);
            add_dependency<PhysicalDevice>(physicalDevice);

            m_imageCI = imageCI;
            m_imageViewCI = imageViewCI;

            m_extent = imageCI.extent;
      }
      void Image::initialize(
            REF(Device) device,
            REF(PhysicalDevice) physicalDevice,
            uint32_t width, uint32_t height,
            VkFormat format,
            VkImageAspectFlags aspect,
            VkImageUsageFlags usageFlags,
            VkImageLayout initialLayout
      )
      {
            base_initialize(device);
            add_dependency<PhysicalDevice>(physicalDevice);

            m_imageCI = fill_image_create_info(
                  width, height, 1,
                  format,
                  usageFlags,
                  initialLayout
            );

            m_imageViewCI = fill_image_view_create_info(
                  m_image,
                  format,
                  aspect
            );
      }
      void Image::create()
      {
            m_device = get_dependency<Device>();

            if (!m_externallyCreated)
            {
                  create_image();
                  allocate_memory();
            }
            create_image_view();

            m_created = true;

            VKH_LOG("Image created")
      }
      void Image::destroy()
      {
            NVE_ASSERT(m_created == true)

            vkDestroyImageView(*m_device, m_imageView, nullptr);
            if (m_memoryAllocated)
                  free_memory();
            if (!m_externallyCreated)
                  vkDestroyImage(*m_device, m_image, nullptr);

            m_created = false;

            VKH_LOG("Image destroyed")
      }
      void Image::create_image()
      {
            NVE_ASSERT(m_created == false)

            auto res = vkCreateImage(*m_device, &m_imageCI, nullptr, &m_image);
            VK_CHECK_ERROR(res)
      }
      void Image::create_image_view()
      {
            NVE_ASSERT(m_created == false)

            m_imageViewCI.image = m_image;
            auto res = vkCreateImageView(*m_device, &m_imageViewCI, nullptr, &m_imageView);
            VK_CHECK_ERROR(res)
      }
      void Image::allocate_memory()
      {
            if (m_memoryAllocated)
                  free_memory();

            auto device = get_dependency<Device>();
            auto physicalDevice = get_dependency<PhysicalDevice>();

            VkMemoryRequirements memoryRequirements;
            vkGetImageMemoryRequirements(*device, m_image, &memoryRequirements);

            auto memoryAI = fill_memory_allocation_info(
                  memoryRequirements.size,
                  find_memory_type(
                        *physicalDevice,
                        memoryRequirements.memoryTypeBits,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                  )
            );
            auto res = vkAllocateMemory(
                  *device,
                  &memoryAI,
                  nullptr,
                  &m_memory
            );
            VK_CHECK_ERROR(res)

            vkBindImageMemory(*device, m_image, m_memory, 0);

            m_memoryAllocated = true;
      }
      void Image::free_memory()
      {
            NVE_ASSERT(m_memoryAllocated == true)

            auto device = get_dependency<Device>();
            vkFreeMemory(*device, m_memory, nullptr);
            m_memoryAllocated = false;
      }
      Image::operator VkImage()
      {
            if (m_recreateImage)
                  create_image();
            if (m_recreateView || m_recreateImage)
                  create_image_view();
            return m_image;
      }
      VkImageView Image::image_view()
      {
            return m_imageView;
      }

      VkExtent3D Image::extent()
      {
            return m_extent;
      }

      VkImageCreateInfo& Image::image_create_info()
      {
            m_recreateImage = true;
            return m_imageCI;
      }
      VkImageViewCreateInfo& Image::image_view_create_info()
      {
            m_recreateView = true;
            return m_imageViewCI;
      }

      // --------------------------------
      // SUBPASS COUNT HANDLER
      // --------------------------------

      void SubpassCountHandler::initialize()
      {
            m_lastSubpasses = 0;
      }
      void SubpassCountHandler::on_update()
      {

      }
      void SubpassCountHandler::on_unresolve()
      {

      }
      bool SubpassCountHandler::check_subpasses()
      {
            uint32_t subpasses = subpass_count();
            if (subpasses != m_lastSubpasses)
            {
                  m_lastSubpasses = subpasses;

                  unresolve();
                  try_update();

                  return true;
            }
            return false;
      }
      void SubpassCountHandler::add_subpass_count_callback(CallbackFunction callback)
      {
            m_callbacks.push_back(callback);
      }
      uint32_t SubpassCountHandler::subpass_count() const
      {
            uint32_t subpassCount{ 0 };
            for (auto callback : m_callbacks)
                  subpassCount += callback();
            return subpassCount;
      }

      // --------------------------------
      // RENDER PASS
      // --------------------------------

      void RenderPass::initialize(
            REF(Device) device,
            REF(PhysicalDevice) physicalDevice,
            REF(Swapchain) swapchain,
            REF(SubpassCountHandler) subpassCountHandler
      )
      {
            add_dependency(device);
            add_dependency(physicalDevice);
            add_dependency(swapchain);
            add_dependency(subpassCountHandler);

            VulkanHandle::initialize();

            m_doSubpassUpdate = true;
      }
      void RenderPass::create()
      {
            auto device = get_dependency<Device>();
            auto physicalDevice = get_dependency<PhysicalDevice>();
            auto swapchain = get_dependency<Swapchain>();
            auto subpassCountHandler = get_dependency<SubpassCountHandler>();

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

            uint32_t subpassCount = subpassCountHandler->subpass_count();
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

            VKH_LOG("Render Pass created")
      }
      void RenderPass::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyRenderPass(*device, m_renderPass, nullptr);

            VKH_LOG("RenderPass destroyed")
      }
      RenderPass::operator VkRenderPass()
      {
            return m_renderPass;
      }

      // --------------------------------
      // FRAMEBUFFER
      // --------------------------------

      void Framebuffer::initialize(REF(Device) device, REF(RenderPass) renderPass, std::vector<REF(Image)> images)
      {
            add_dependency(device);
            add_dependency(renderPass);
            add_dependencies(images);

            VulkanHandle::initialize();
      }
      void Framebuffer::create()
      {
            auto renderPass = get_dependency<RenderPass>();
            auto device = get_dependency<Device>();
            auto images = get_dependencies<Image>();

            std::vector<VkImageView> attachments(images.size());
            for (size_t i = 0; i < images.size(); i++)
                  attachments[i] = images[i]->image_view();

            VkFramebufferCreateInfo framebufferCI;
            framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferCI.flags = 0;
            framebufferCI.pNext = nullptr;
            framebufferCI.renderPass = *renderPass;
            framebufferCI.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferCI.pAttachments = attachments.data();
            framebufferCI.width = images.front()->extent().width;
            framebufferCI.height = images.front()->extent().height;
            framebufferCI.layers = 1;

            auto res = vkCreateFramebuffer(*device, &framebufferCI, nullptr, &m_framebuffer);
            VK_CHECK_ERROR(res)

            VKH_LOG("Framebuffer created")
      }
      void Framebuffer::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyFramebuffer(*device, m_framebuffer, nullptr);

            VKH_LOG("Framebuffer destroyed")
      }
      Framebuffer::operator VkFramebuffer()
      {
            return m_framebuffer;
      }

      // --------------------------------
      // COMMAND POOL
      // --------------------------------

      void CommandPool::initialize(REF(Device) device)
      {
            add_dependency(device);

            VulkanHandle::initialize();
      }
      void CommandPool::create()
      {
            auto device = get_dependency<Device>();

            VkCommandPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            createInfo.pNext = nullptr;
            createInfo.queueFamilyIndex = device->transfer_queue_family();

            auto res = vkCreateCommandPool(*device, &createInfo, nullptr, &m_commandPool);
            VK_CHECK_ERROR(res)

            VKH_LOG("Command Pool created")
      }
      void CommandPool::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyCommandPool(*device, m_commandPool, nullptr);

            VKH_LOG("Command Pool destroyed")
      }
      CommandPool::operator VkCommandPool()
      {
            return m_commandPool;
      }

      // --------------------------------
      // COMMAND BUFFERS
      // --------------------------------

      void CommandBuffers::initialize(REF(Device) device, REF(CommandPool) commandPool, uint32_t count, VkCommandBufferLevel level)
      {
            add_dependency(device);
            add_dependency(commandPool);

            m_commandBuffers.resize(count);
            m_level = level;

            VulkanHandle::initialize();
      }
      void CommandBuffers::create()
      {
            auto device = get_dependency<Device>();
            auto commandPool = get_dependency<CommandPool>();

            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.pNext = nullptr;
            allocInfo.commandPool = *commandPool;
            allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
            allocInfo.level = m_level;

            auto res = vkAllocateCommandBuffers(*device, &allocInfo, m_commandBuffers.data());
            VK_CHECK_ERROR(res)

            VKH_LOG("Command Buffers created")
      }
      void CommandBuffers::destroy()
      {
            auto device = get_dependency<Device>();
            auto commandPool = get_dependency<CommandPool>();

            vkFreeCommandBuffers(*device, *commandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());

            VKH_LOG("Command Buffers destroyed")
      }
      VkCommandBuffer CommandBuffers::get_command_buffer(uint32_t index)
      {
            return get_command_buffers()[index];
      }
      const std::vector<VkCommandBuffer>& CommandBuffers::get_command_buffers()
      {
            return m_commandBuffers;
      }

      // --------------------------------
      // SEMAPHORE
      // --------------------------------

      void Semaphore::initialize(REF(Device) device, SemaphoreType type)
      {
            add_dependency(device);

            m_type = type;

            VulkanHandle::initialize();
      }
      void Semaphore::create()
      {
            auto device = get_dependency<Device>();

            VkSemaphoreCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            switch (m_type)
            {
            case vk::Semaphore::Binary:
                  createInfo.flags = VK_SEMAPHORE_TYPE_BINARY;
                  break;
            case vk::Semaphore::Timeline:
                  createInfo.flags = VK_SEMAPHORE_TYPE_TIMELINE;
                  break;
            default:
                  break;
            }

            auto res = vkCreateSemaphore(*device, &createInfo, nullptr, &m_semaphore);
            VK_CHECK_ERROR(res)

            VKH_LOG("Semaphore created")
      }
      void Semaphore::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroySemaphore(*device, m_semaphore, nullptr);

            VKH_LOG("Semaphore destroyed")
      }
      Semaphore::operator VkSemaphore()
      {
            return m_semaphore;
      }

      // --------------------------------
      // FENCE
      // --------------------------------

      void Fence::initialize(REF(Device) device, bool createSignaled)
      {
            add_dependency(device);
            m_createSignaled = createSignaled;
      }
      void Fence::create()
      {
            auto device = get_dependency<Device>();

            VkFenceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;

            if (m_createSignaled)
                  createInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;

            auto res = vkCreateFence(*device, &createInfo, nullptr, &m_fence);
            VK_CHECK_ERROR(res)

            VKH_LOG("Fence created")
      }
      void Fence::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyFence(*device, m_fence, nullptr);

            VKH_LOG("Fence destroyed")
      }
      Fence::operator VkFence()
      {
            return m_fence;
      }

      void Fence::wait()
      {
            auto device = get_dependency<Device>();

            vkWaitForFences(*device, 1, &m_fence, VK_TRUE, UINT64_MAX);
            vkResetFences(*device, 1, &m_fence);
      }

      // --------------------------------
      // DEBUG UTILS MESSENGER
      // --------------------------------

      // --------------------------------
      // DESCRIPTOR POOL
      // --------------------------------

      const std::unordered_map<VkDescriptorType, uint32_t> DescriptorPool::s_defaultPoolSizes = 
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
      void DescriptorPool::create_vk_pool_sizes(VkDescriptorPoolSize*& poolSizes, uint32_t& poolSizeCount, const std::unordered_map<VkDescriptorType, uint32_t>& poolSizeMap)
      {
            poolSizeCount = static_cast<uint32_t>(poolSizeMap.size());
            poolSizes = new VkDescriptorPoolSize[poolSizeMap.size()];

            size_t index = 0;
            for (auto [descriptorType, descriptorCount] : poolSizeMap)
            {
                  poolSizes[index].type = descriptorType;
                  poolSizes[index].descriptorCount = descriptorCount;
                  index++;
            }
      }

      void DescriptorPool::initialize(REF(Device) device, std::unordered_map<VkDescriptorType, uint32_t> poolSizes, uint32_t maxSets)
      {
            add_dependency(device);
            m_poolSizes = poolSizes;
            m_maxSets = maxSets;

            VulkanHandle::initialize();
      }
      void DescriptorPool::create()
      {
            auto device = get_dependency<Device>();

            VkDescriptorPoolCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

            VkDescriptorPoolSize* poolSizes{ nullptr };
            uint32_t poolSizeCount{ 0 };
            if (m_poolSizes.empty())
                  create_vk_pool_sizes(poolSizes, poolSizeCount, s_defaultPoolSizes);
            else
                  create_vk_pool_sizes(poolSizes, poolSizeCount, m_poolSizes);

            createInfo.maxSets = m_maxSets * poolSizeCount;
            createInfo.poolSizeCount = poolSizeCount;
            createInfo.pPoolSizes = poolSizes;

            auto res = vkCreateDescriptorPool(*device, &createInfo, nullptr, &m_descriptorPool);
            VK_CHECK_ERROR(res)

            delete[] poolSizes;

            VKH_LOG("Descriptor Pool created")
      }
      void DescriptorPool::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyDescriptorPool(*device, m_descriptorPool, nullptr);

            VKH_LOG("Descriptor Pool destroyed")
      }
      DescriptorPool::operator VkDescriptorPool()
      {
            return m_descriptorPool;
      }

      // ------------------------------
      // DEPENDENCY IDS
      // ------------------------------
      DependencyId Instance::dependency_id() { return "Instance"; }
      DependencyId Window::dependency_id() { return "Window"; }
      DependencyId Surface::dependency_id() { return "Surface"; }
      DependencyId Swapchain::dependency_id() { return "Swapchain"; }
      DependencyId Device::dependency_id() { return "Device"; }
      DependencyId PhysicalDevice::dependency_id() { return "PhysicalDevice"; }
      DependencyId Image::dependency_id() { return "Image"; }
      DependencyId RenderPass::dependency_id() { return "RenderPass"; }
      DependencyId Framebuffer::dependency_id() { return "Framebuffer"; }
      DependencyId SubpassCountHandler::dependency_id() { return "SubpassCountHandler"; }
      DependencyId CommandPool::dependency_id() { return "CommandPool"; }
      DependencyId CommandBuffers::dependency_id() { return "CommandBuffers"; }
      DependencyId DescriptorPool::dependency_id() { return "DescriptorPool"; }
      DependencyId Semaphore::dependency_id() { return "Semaphore"; }
      DependencyId Fence::dependency_id() { return "Fence"; }
      DependencyId Queue::dependency_id() { return "Queue"; }

}; // namespace vk