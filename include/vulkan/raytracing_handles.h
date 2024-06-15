#pragma once

#include <vulkan/vulkan.h>

#include "vulkan/vulkan_handles.h"

namespace vk
{
      class AccelerationStructure : public VulkanHandle
      {
      public:

            void initialize(
                  REF(Device) device,
                  VkAccelerationStructureTypeKHR type
            );

      protected:

            void create() override;
            void destroy() override;

            DependencyId dependency_id() override;

      private:

            PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;

            VkAccelerationStructureKHR m_accelerationStructure;

            VkAccelerationStructureTypeKHR m_type;

            VkBuffer m_buffer;
            void create_buffer();

            VkDeviceSize m_offset;
            VkDeviceSize m_size;

      };

      class CompleteAccelerationStructure : public Dependency
      {
      public:
            void initialize(REF(Device) device, REF(CommandPool) commandPool, REF(Queue) queue);

            void build();

            DependencyId dependency_id() override;

      private:

            VkAccelerationStructureCreateInfoKHR create_info();

            AccelerationStructure m_toplevelAccelerationStructure;
            AccelerationStructure m_lowlevelAccelerationStructure;

      };
} // namespace vk