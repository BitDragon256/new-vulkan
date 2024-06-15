#include "vulkan/raytracing_handles.h"

#include "vulkan/vulkan_helpers.h"

namespace vk
{
      void AccelerationStructure::initialize(
            REF(Device) device,
            VkAccelerationStructureTypeKHR type
      )
      {
            add_dependency(device);

            m_type = type;

            VulkanHandle::initialize();
      }

      void AccelerationStructure::create()
      {
            auto device = get_dependency<Device>();

            create_buffer();
           
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;

            createInfo.createFlags = 0;
            createInfo.buffer = m_buffer;
            createInfo.offset = m_offset;
            createInfo.size = m_size;
            createInfo.type = m_type;
            createInfo.deviceAddress = 0; // accelerationStructureCaptureReplay is not used
            
            auto res = vkCreateAccelerationStructureKHR(
                  *device,
                  &createInfo,
                  nullptr,
                  &m_accelerationStructure
            );
            VK_CHECK_ERROR(res)
      }
      void AccelerationStructure::destroy()
      {
            auto device = get_dependency<Device>();
            vkDestroyAccelerationStructureKHR(
                  *device,
                  m_accelerationStructure,
                  nullptr
            );
      }

      void AccelerationStructure::create_buffer()
      {
            VkBufferCreateInfo bufferCI = {};
            bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferCI.pNext = nullptr;
            bufferCI.flags = 0;
            bufferCI.size;
            bufferCI.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
            bufferCI.sharingMode;

            auto device = get_dependency<Device>();
            auto res = vkCreateBuffer(
                  *device,
                  &bufferCI,
                  nullptr,
                  &m_buffer
            );
            VK_CHECK_ERROR(res)
      }

      // --------------------------------------
      // COMPLETE ACCELERATION STRUCTURE
      // --------------------------------------

      void CompleteAccelerationStructure::build()
      {
            auto device = get_dependency<Device>();
            auto commandPool = get_dependency<CommandPool>();
            auto queue = get_dependency<Queue>();

            VkCommandBuffer commandBuffer = begin_single_time_cmd_buffer(*commandPool, *device);

            vkCmdBuildAccelerationStructuresKHR(
                  commandBuffer,
                  1,
                  &
            )

            end_single_time_cmd_buffer(commandBuffer, *commandPool, *device, *queue);
      }

      VkAccelerationStructureCreateInfoKHR CompleteAccelerationStructure::create_info()
      {
            VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo = {};
            buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildGeometryInfo.pNext = nullptr;
            buildGeometryInfo.type; // VkAccelerationStructureTypeKHR
            buildGeometryInfo.flags; // VkBuildAccelerationStructureFlagsKHR
            buildGeometryInfo.mode; // VkBuildAccelerationStructureModeKHR
            buildGeometryInfo.srcAccelerationStructure; // VkAccelerationStructureKHR
            buildGeometryInfo.dstAccelerationStructure; // VkAccelerationStructureKHR
            buildGeometryInfo.geometryCount; // uint32_t
            buildGeometryInfo.pGeometries; // const VkAccelerationStructureGeometryKHR*
            buildGeometryInfo.ppGeometries; // const VkAccelerationStructureGeometryKHR* const*
            buildGeometryInfo.scratchData; // VkDeviceOrHostAddressKHR
      }

} // namespace vk