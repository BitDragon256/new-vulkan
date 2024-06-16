#pragma once

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "vulkan/vulkan_handles.h"

// forward declarations
struct RayTracingModel;

namespace vk
{
      typedef struct AccelerationStructureTriangleMesh
      {
            VkDeviceOrHostAddressConstKHR vertexData;
            uint32_t vertexCount;
            VkDeviceSize vertexStride;

            VkDeviceOrHostAddressConstKHR indexData;
            uint32_t indexCount;

            VkDeviceOrHostAddressConstKHR transformData;
      } AccelerationStructureTriangleMesh;

      typedef struct AccelerationStructureBuffer
      {
            VkBuffer buffer;
            VkDeviceMemory memory;
            VkDeviceAddress deviceAddress;
            VkDeviceSize size;
      } AccelerationStructureBuffer;

      class AccelerationStructure : public VulkanHandle
      {
      public:

            void initialize(
                  REF(Device) device,
                  VkAccelerationStructureTypeKHR type
            );

            VkAccelerationStructureBuildGeometryInfoKHR get_build_info();
            VkAccelerationStructureBuildRangeInfoKHR* get_range_info();

            void set_triangle_data(const std::vector<AccelerationStructureTriangleMesh>& meshes);
            void set_instance_data(const std::vector<REF(AccelerationStructure)>& accelerationStructures);

            VkDeviceAddress device_address();

      protected:

            void create() override;
            void destroy() override;

            DependencyId dependency_id() override;

      private:

            PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;

            VkAccelerationStructureKHR m_accelerationStructure;

            VkAccelerationStructureTypeKHR m_type;

            AccelerationStructureBuffer m_mainBuffer;

            void create_main_buffer();
            void destroy_main_buffer();

            AccelerationStructureBuffer m_scratchBuffer;

            void create_scratch_buffer();
            void destroy_scratch_buffer();

            std::vector<VkAccelerationStructureGeometryKHR> m_geometry;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> m_rangeInfos;

            VkAccelerationStructureBuildGeometryInfoKHR m_buildInfo;

            VkAccelerationStructureBuildSizesInfoKHR query_required_size();
      };

      class CompleteAccelerationStructure : public VulkanHandle
      {
      public:
            void initialize(REF(Device) device, REF(CommandPool) commandPool, REF(Queue) queue);
            void set_geometry(const std::vector<RayTracingModel>& models);

            void create() override;
            void destroy() override;

            void build(const std::vector<AccelerationStructureTriangleMesh>& meshes);

            DependencyId dependency_id() override;

      private:

            AccelerationStructure m_toplevelAccelerationStructure;
            std::vector<AccelerationStructure> m_lowlevelAccelerationStructures;

      };
} // namespace vk