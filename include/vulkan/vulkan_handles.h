#pragma once

#include <string>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "dependency.h"
#include "vulkan/vulkan_helpers.h"

namespace vk
{

      class VulkanHandle : public Dependency
      {
      public:
            VulkanHandle();
            virtual void create() = 0;
            virtual void destroy() = 0;

            void on_update() override;

      private:
            bool m_created;
      };

      class Instance : public VulkanHandle
      {
      public:
            void initialize(std::string applicationName, uint32_t applicationVersion, std::string engineName, bool enableValidationLayers = true);
            void create() override;
            void destroy() override;
            operator VkInstance();

            std::string m_applicationName, m_engineName;
            uint32_t m_applicationVersion;
            bool m_enableValidationLayers;

            std::vector<const char*> m_instanceLayers;

      private:

            VkInstance m_instance;
      };
      class PhysicalDevice : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkPhysicalDevice();

      private:

            VkPhysicalDevice m_physicalDevice;
      };
      class Device : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkDevice();

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
            void create() override;
            void destroy() override;
            operator GLFWwindow* ();

      private:

            GLFWwindow* m_window;
      };
      class Surface : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkSurfaceKHR();

      private:

            VkSurfaceKHR m_surface;
      };
      class Image : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            void create(Reference<Device> device, VkImage image, const VkImageViewCreateInfo& imageViewCI);
            void create(Reference<Device> device, const VkImageCreateInfo& imageCI, const VkImageViewCreateInfo& imageViewCI);
            void destroy();
            operator VkImage();

            VkImageView m_imageView;
            VkExtent3D m_extent;

            struct ImageCreateInfo : public Dependency
            {
                  VkImageViewCreateInfo imageView;
                  VkImageCreateInfo image;
            };

      private:

            void create_image(const VkImageCreateInfo& imageCI);
            void create_image_view(const VkImageViewCreateInfo& imageViewCI);

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

      private:

            VkCommandPool m_cmdPool;
      };
      class CommandBuffer : public VulkanHandle
      {
      public:
            void create() override;
            void destroy() override;
            operator VkCommandBuffer();

      private:

            VkCommandBuffer m_cmdBuffer;
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