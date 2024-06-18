#pragma once

#include <functional>
#include <string>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "dependency.h"
#include "vulkan/vulkan_helpers.h"

#define TYPE(T) typeid(T).name()

namespace vk
{

      class VulkanHandle : public Dependency
      {
      public:

            VulkanHandle();

      protected:

            virtual void create() = 0;
            virtual void destroy() = 0;

            void on_update() override;
            void on_unresolve() override;
            void initialize();

            bool m_created;

      private:
            bool m_initialized;
      };

      // forward declarations
      class Surface;
      class Semaphore;
      class Fence;
      class Framebuffer;
      class RenderPass;

      class Instance : public VulkanHandle
      {
      public:
            void initialize(
                  std::string applicationName,
                  uint32_t applicationVersion,
                  std::string engineName,
                  bool enableValidationLayers = true
            );
            operator VkInstance();

            std::string m_applicationName, m_engineName;
            uint32_t m_applicationVersion;
            bool m_enableValidationLayers;

            std::vector<const char*> m_instanceLayers;

            DependencyId dependency_id() override;

      protected:
            void create() override;
            void destroy() override;

      private:

            VkInstance m_instance;
      };
      class PhysicalDevice : public VulkanHandle
      {
      public:
            void initialize(REF(Instance) instance, REF(Surface) surface);
            operator VkPhysicalDevice();

            DependencyId dependency_id() override;
      protected:
            void create() override;
            void destroy() override;

      private:

            VkPhysicalDevice m_physicalDevice;
      };
      class Queue : public VulkanHandle
      {
      public:
            void initialize(VkQueue queue);

            VkQueue& get();
            operator VkQueue();

            void submit(const VkSubmitInfo& submitInfo, REF(Fence) fence);
            void submit(const std::vector<VkSubmitInfo>& submits);
            void submit(const std::vector<VkSubmitInfo>& submits, REF(Fence) fence);

            void present(const VkPresentInfoKHR& presentInfo);
            void submit_command_buffers(
                  std::vector<VkCommandBuffer>& commandBuffers,
                  std::vector<REF(Semaphore)>& waitSemaphores,
                  std::vector<VkPipelineStageFlags>& waitStages,
                  std::vector<REF(Semaphore)>& signalSemaphores,
                  REF(Fence) fence
            );

            DependencyId dependency_id() override;

      protected:
            void create() override;
            void destroy() override;

      private:

            VkQueue m_queue;
      };
      class Device : public VulkanHandle
      {
      public:
            void initialize(REF(PhysicalDevice) physicalDevice, REF(Surface) surface, std::vector<const char*> extensions);
            operator VkDevice();

            uint32_t graphics_queue_family();
            uint32_t presentation_queue_family();
            uint32_t transfer_queue_family();
            uint32_t compute_queue_family();

            Queue m_graphicsQueue;
            Queue m_presentationQueue;
            Queue m_transferQueue;
            Queue m_computeQueue;

            uint32_t memory_type_index(uint32_t memoryTypeBits, VkMemoryPropertyFlags propertyFlags);

            DependencyId dependency_id() override;
      protected:
            void create() override;
            void destroy() override;

      private:

            QueueFamilyIndices m_queueFamilyIndices;
            
            VkDevice m_device;

            std::vector<const char*> m_extensions;
      };
      class Window : public VulkanHandle
      {
      public:
            void initialize(int width, int height, std::string title);
            operator GLFWwindow* ();

            DependencyId dependency_id() override;

      protected:
            void create() override;
            void destroy() override;
      private:

            GLFWwindow* m_window;

            int m_height, m_width;
            std::string m_title;
      };
      class Surface : public VulkanHandle
      {
      public:
            void initialize(REF(Instance) instance, REF(Window) window);
            operator VkSurfaceKHR();

            DependencyId dependency_id() override;

      protected:
            void create() override;
            void destroy() override;
      private:

            VkSurfaceKHR m_surface;
      };
      /// <summary>
      /// A wrapper class for VkImage and VkImageView.
      /// On a call of create(), Image creates both vulkan handles with the specified specs.
      /// </summary>
      class Image : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  VkImage image,
                  VkExtent3D extent,
                  VkFormat format,
                  VkImageAspectFlags aspect
            );
            void initialize(
                  REF(Device) device,
                  REF(PhysicalDevice) physicalDevice,
                  const VkImageCreateInfo& imageCI,
                  const VkImageViewCreateInfo& imageViewCI
            );
            void initialize(
                  REF(Device) device,
                  REF(PhysicalDevice) physicalDevice,
                  uint32_t width, uint32_t height,
                  VkFormat format,
                  VkImageAspectFlags aspectFlags,
                  VkImageUsageFlags usageFlags,
                  VkImageLayout initialLayout
            );
            void create() override;
            void destroy() override;
            operator VkImage();
            VkImageView image_view();
            VkExtent3D extent();

            struct ImageCreateInfo : public Dependency
            {
                  VkImageViewCreateInfo imageView;
                  VkImageCreateInfo image;
            };

            VkImageCreateInfo& image_create_info();
            VkImageViewCreateInfo& image_view_create_info();

            DependencyId dependency_id() override;

      private:

            void base_initialize(REF(Device) device);

            void create_image();
            void create_image_view();
            void allocate_memory();
            void free_memory();

            bool m_memoryAllocated;

            VkImageCreateInfo m_imageCI;
            VkImageViewCreateInfo m_imageViewCI;

            VkExtent3D m_extent;

            VkImage m_image;
            VkImageView m_imageView;
            VkDeviceMemory m_memory;

            bool m_recreateImage;
            bool m_recreateView;

            bool m_externallyCreated;

            Reference<Device> m_device;

            VkFormat m_format;
      };
      class Swapchain : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  REF(PhysicalDevice) physicalDevice,
                  REF(Window) window,
                  REF(Surface) surface,
                  REF(RenderPass) renderPass
            );
            void create() override;
            void destroy() override;
            operator VkSwapchainKHR();

            VkFormat m_imageFormat;
            VkExtent2D m_extent;

            std::vector<Image> m_images;
            std::vector<Image> m_depthImages;
            std::vector<Framebuffer> m_framebuffers;

            uint32_t size();

            uint32_t m_currentImageIndex;
            uint32_t m_frameObectIndex;
            uint32_t m_lastFrameObjectIndex;

            VkResult next_image();
            void next_frame();
            void present_current_image(std::vector<REF(Semaphore)>& semaphores);

            REF(Semaphore) current_image_available_semaphore();

            DependencyId dependency_id() override;

      private:

            VkSwapchainKHR m_swapchain;

            std::vector<Semaphore> m_imageAvailableSemaphores;

            REF(RenderPass) m_renderPass;
      };

      class SubpassCountHandler : public Dependency
      {
            using CallbackFunction = std::function<uint32_t(void)>;
      public:
            void initialize();

            void on_update() override;
            void on_unresolve() override;

            bool check_subpasses();

            uint32_t subpass_count() const;
            void add_subpass_count_callback(CallbackFunction callback);

            DependencyId dependency_id() override;

      private:
            std::vector<CallbackFunction> m_callbacks;

            uint32_t m_lastSubpasses;
      };

      class RenderPass : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  REF(PhysicalDevice) physicalDevice,
                  REF(Swapchain) swapchain,
                  REF(SubpassCountHandler) subpassCountHandler
            );
            void create() override;
            void destroy() override;
            operator VkRenderPass();

            DependencyId dependency_id() override;

      private:

            VkRenderPass m_renderPass;

            bool m_doSubpassUpdate;

      };
      class Framebuffer : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  REF(RenderPass) renderPass,
                  std::vector<REF(Image)> images
            );
            void create() override;
            void destroy() override;
            operator VkFramebuffer();

            DependencyId dependency_id() override;

      private:

            VkFramebuffer m_framebuffer;
      };
      class CommandPool : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device
            );
            void create() override;
            void destroy() override;
            operator VkCommandPool();

            DependencyId dependency_id() override;

      private:

            VkCommandPool m_commandPool;
      };
      class CommandBuffers : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  REF(CommandPool) commandPool,
                  uint32_t count,
                  VkCommandBufferLevel level
            );
            void initialize(
                  REF(Device) device,
                  REF(CommandPool) commandPool,
                  std::function<uint32_t(void)> countCallback,
                  VkCommandBufferLevel level
            );
            void create() override;
            void destroy() override;
            VkCommandBuffer get_command_buffer(uint32_t index);
            const std::vector<VkCommandBuffer>& get_command_buffers();

            DependencyId dependency_id() override;

      private:

            std::vector<VkCommandBuffer> m_commandBuffers;
            std::function<uint32_t(void)> m_countCallback;
            VkCommandBufferLevel m_level;
      };
      class Semaphore : public VulkanHandle
      {
      public:
            enum SemaphoreType {
                  Binary = VK_SEMAPHORE_TYPE_BINARY,
                  Timeline = VK_SEMAPHORE_TYPE_TIMELINE
            };

            void initialize(
                  REF(Device) device,
                  SemaphoreType type = SemaphoreType::Binary
            );
            void create() override;
            void destroy() override;
            operator VkSemaphore();

            DependencyId dependency_id() override;

      private:

            VkSemaphore m_semaphore;
            SemaphoreType m_type;
      };
      class Fence : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  bool createSignaled = false
            );
            void create() override;
            void destroy() override;
            operator VkFence();

            DependencyId dependency_id() override;

            void wait();

      private:

            VkFence m_fence;
            bool m_createSignaled;
      };
      class DescriptorPool : public VulkanHandle
      {
      public:
            void initialize(
                  REF(Device) device,
                  std::unordered_map<VkDescriptorType, uint32_t> poolSizes = s_defaultPoolSizes,
                  uint32_t maxSets = 1000
            );
            void create() override;
            void destroy() override;
            operator VkDescriptorPool();

            static const std::unordered_map<VkDescriptorType, uint32_t> s_defaultPoolSizes;

            DependencyId dependency_id() override;

      private:

            VkDescriptorPool m_descriptorPool;
            std::unordered_map<VkDescriptorType, uint32_t> m_poolSizes;
            uint32_t m_maxSets;

            static void create_vk_pool_sizes(VkDescriptorPoolSize*& poolSizes, uint32_t& poolSizeCount, const std::unordered_map<VkDescriptorType, uint32_t>& poolSizeMap);
      };


      class SimpleBuffer : private Dependency
      {
      public:

            void create(
                  REF(Device) device,
                  VkDeviceSize size,
                  VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags memoryProperties,
                  VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE
            );
            void map_memory();
            void unmap_memory();

            VkBuffer m_buffer;
            VkDeviceMemory m_memory;
            void* m_mappedMemory;

            DependencyId dependency_id() override;
            operator VkBuffer();

      private:

            VkDeviceSize m_size;
            VkBufferUsageFlags m_usage;
            VkMemoryPropertyFlags m_memoryProperties;
            VkSharingMode m_sharingMode;

            bool m_memoryIsMapped;

            void on_update() override;
            void on_unresolve() override;
      };

}; // vk