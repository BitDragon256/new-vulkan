#pragma once

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
            void initialize();

      private:
            bool m_created;
            bool m_initialized;
      };

      // forward declarations
      class Surface;

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
      protected:
            void create() override;
            void destroy() override;

      private:

            VkPhysicalDevice m_physicalDevice;
      };
      class Device : public VulkanHandle
      {
      public:
            void initialize(REF(PhysicalDevice) physicalDevice, REF(Surface) surface);
            operator VkDevice();
      protected:
            void create() override;
            void destroy() override;

            QueueFamilyIndices m_queueFamilyIndices;
            
            VkQueue m_graphicsQueue;
            VkQueue m_presentationQueue;
            VkQueue m_transferQueue;
            VkQueue m_computeQueue;

      private:

            VkDevice m_device;
      };
      class Window : public VulkanHandle
      {
      public:
            void initialize(int width, int height, std::string title);
            operator GLFWwindow* ();

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
            void initialize(REF(Device) device);
            void initialize(REF(Device) device, VkImage image, const VkImageViewCreateInfo& imageViewCI);
            void initialize(REF(Device) device, const VkImageCreateInfo& imageCI, const VkImageViewCreateInfo& imageViewCI);
            void create() override;
            void destroy() override;
            operator VkImage();

            VkImageView m_imageView;
            VkExtent3D m_extent;

            struct ImageCreateInfo : public Dependency
            {
                  VkImageViewCreateInfo imageView;
                  VkImageCreateInfo image;
            };

            static VkImageCreateInfo default_image_create_info();
            static VkImageViewCreateInfo default_image_view_create_info();

      private:

            void create_image(const VkImageCreateInfo& imageCI);
            void create_image_view(const VkImageViewCreateInfo& imageViewCI);

            VkImageCreateInfo m_imageCI;
            VkImageViewCreateInfo m_imageViewCI;
            bool m_onlyCreateImageView;

            VkImage m_image;
            Reference<Device> m_device;
      };
      class Swapchain : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkSwapchainKHR();

            VkFormat m_imageFormat;
            VkExtent2D m_extent;

            std::vector<Image> m_images;

      private:

            VkImageViewCreateInfo swapchain_image_view_create_info(uint32_t index);

            VkSwapchainKHR m_swapchain;
      };
      class RenderPass : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkRenderPass();

      private:

            VkRenderPass m_renderPass;
      };
      class Framebuffer : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkFramebuffer();

      private:

            VkFramebuffer m_framebuffer;
      };
      class CommandPool : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkCommandPool();

            struct CommandPoolCreateInfo : public Dependency
            {
                  uint32_t queueFamilyIndex;
            };

      private:

            VkCommandPool m_commandPool;
      };
      class CommandBuffer : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkCommandBuffer();

      private:

            VkCommandBuffer m_commandBuffer;
      };
      class Semaphore : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkSemaphore();

      private:

            VkSemaphore m_semaphore;
      };
      class Fence : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkFence();

      private:

            VkFence m_fence;
      };
      class DescriptorPool : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkDescriptorPool();

      private:

            VkDescriptorPool m_descriptorPool;
      };

}; // vk