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

            set_vulkan_function_pointers();

            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.pNext = nullptr;

            createInfo.createFlags = 0;
            createInfo.buffer = m_mainBuffer.buffer;
            createInfo.offset = 0;
            createInfo.size = m_mainBuffer.size;
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

      void AccelerationStructure::create_main_buffer()
      {
            m_mainBuffer.buffer = make_buffer(
                  get_dependency<Device>(),
                  m_mainBuffer.size,
                  VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                  VK_SHARING_MODE_EXCLUSIVE
            );
      }
      void AccelerationStructure::destroy_main_buffer()
      {
            auto device = get_dependency<Device>();
            free_device_memory(device, m_mainBuffer.memory);
            destroy_buffer(device, m_mainBuffer.buffer);
      }
      void AccelerationStructure::create_scratch_buffer()
      {
            auto device = get_dependency<Device>();

            m_scratchBuffer.buffer = make_buffer(
                  device,
                  m_scratchBuffer.size,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                  VK_SHARING_MODE_EXCLUSIVE
            );
            m_scratchBuffer.memory = allocate_buffer_memory(
                  device,
                  m_scratchBuffer.buffer,
                  VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
            );
            VK_CHECK_ERROR( vkBindBufferMemory(*device, m_scratchBuffer.buffer, m_scratchBuffer.memory, 0) )

            m_scratchBuffer.deviceAddress = get_buffer_device_address(device, m_scratchBuffer.buffer);
      }
      void AccelerationStructure::destroy_scratch_buffer()
      {
            auto device = get_dependency<Device>();

            free_device_memory(device, m_scratchBuffer.memory);
            destroy_buffer(device, m_scratchBuffer.buffer);
      }

      VkAccelerationStructureBuildGeometryInfoKHR AccelerationStructure::get_build_info()
      {
            m_buildInfo = {};
            m_buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            m_buildInfo.pNext = nullptr;
            m_buildInfo.type = m_type; // VkAccelerationStructureTypeKHR
            m_buildInfo.flags = 0; // VkBuildAccelerationStructureFlagsKHR
            m_buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR; // VkBuildAccelerationStructureModeKHR
            m_buildInfo.srcAccelerationStructure = VK_NULL_HANDLE; // VkAccelerationStructureKHR
            m_buildInfo.dstAccelerationStructure = m_accelerationStructure; // VkAccelerationStructureKHR
            m_buildInfo.geometryCount = static_cast<uint32_t>(m_geometry.size()); // uint32_t
            m_buildInfo.pGeometries = m_geometry.data(); // const VkAccelerationStructureGeometryKHR*
            m_buildInfo.ppGeometries = nullptr; // const VkAccelerationStructureGeometryKHR* const*

            auto size = query_required_size();
            m_mainBuffer.size = size.accelerationStructureSize;
            m_scratchBuffer.size = size.buildScratchSize;

            create_main_buffer();
            create_scratch_buffer();

            m_buildInfo.scratchData.deviceAddress = m_scratchBuffer.deviceAddress; // VkDeviceOrHostAddressKHR

            return m_buildInfo;
      }
      VkAccelerationStructureBuildRangeInfoKHR* AccelerationStructure::get_range_info()
      {
            return m_rangeInfos.data();
      }
      void AccelerationStructure::set_triangle_data(
            const std::vector<AccelerationStructureTriangleMesh>& meshes
      )
      {
            NVE_ASSERT(m_type & VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)

            m_geometry.reserve(meshes.size());

            for (auto meshIt = meshes.cbegin() ; meshIt != meshes.cend(); meshIt++)
            {
                  VkAccelerationStructureGeometryKHR geom = {};
                  geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                  geom.pNext = nullptr;
                  geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                  geom.flags = 0;

                  VkAccelerationStructureGeometryTrianglesDataKHR triangleData = {};
                  triangleData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                  triangleData.pNext = nullptr;
                  triangleData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                  triangleData.vertexData = meshIt->vertexData;
                  triangleData.vertexStride = meshIt->vertexStride;
                  triangleData.maxVertex = meshIt->vertexCount - 1;
                  triangleData.indexType = NVE_INDEX_TYPE;
                  triangleData.indexData = meshIt->indexData;
                  triangleData.transformData = meshIt->transformData;

                  geom.geometry.triangles = triangleData;

                  m_geometry.push_back(geom);

                  VkAccelerationStructureBuildRangeInfoKHR rangeInfo = {};
                  rangeInfo.primitiveCount = meshIt->indexCount / 3;
                  rangeInfo.primitiveOffset = meshIt->indexOffset; // index offset
                  rangeInfo.firstVertex = 0;
                  rangeInfo.transformOffset = 0;

                  m_rangeInfos.push_back(rangeInfo);
            }
      }
      void AccelerationStructure::set_instance_data(const std::vector<REF(AccelerationStructure)>& accelerationStructures)
      {
            NVE_ASSERT(m_type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)

            m_geometry.resize(1);
            m_rangeInfos.resize(1);

            // acceleration structure addresses
            std::vector<VkDeviceOrHostAddressKHR> accelerationStructureAddresses;
            accelerationStructureAddresses.reserve(accelerationStructures.size());

            for (auto asIt = accelerationStructures.cbegin(); asIt != accelerationStructures.cend(); asIt++)
            {
                  VkDeviceOrHostAddressKHR accelerationStructureAddress = {};
                  accelerationStructureAddress.deviceAddress = asIt->get()->device_address();
                  accelerationStructureAddresses.push_back(accelerationStructureAddress);
            }

            // geometry
            auto& geometry = m_geometry.front();

            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.pNext = nullptr;
            geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            geometry.flags = 0;

            VkAccelerationStructureGeometryInstancesDataKHR instanceData = {};
            instanceData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            instanceData.pNext = nullptr;
            instanceData.arrayOfPointers = VK_TRUE;
            instanceData.data.hostAddress = static_cast<void*>(accelerationStructureAddresses.data());

            geometry.geometry.instances = instanceData;

            // range info
            auto& rangeInfo = m_rangeInfos.front();

            rangeInfo.primitiveCount = static_cast<uint32_t>(accelerationStructures.size());
            rangeInfo.primitiveOffset = 0;
      }

      VkDeviceAddress AccelerationStructure::device_address()
      {
            NVE_ASSERT(m_created == true)

            VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.pNext = nullptr;
            addressInfo.accelerationStructure = m_accelerationStructure;

            auto device = get_dependency<Device>();
            return vkGetAccelerationStructureDeviceAddressKHR(
                  *device,
                  &addressInfo
            );
      }
      void AccelerationStructure::set_vulkan_function_pointers()
      {
            auto device = get_dependency<Device>();
            vkCreateAccelerationStructureKHR = reinterpret_cast<decltype(vkCreateAccelerationStructureKHR)>(vkGetDeviceProcAddr(*device, "vkCreateAccelerationStructureKHR"));
            vkDestroyAccelerationStructureKHR = reinterpret_cast<decltype(vkDestroyAccelerationStructureKHR)>(vkGetDeviceProcAddr(*device, "vkDestroyAccelerationStructureKHR"));
            vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<decltype(vkGetAccelerationStructureDeviceAddressKHR)>(vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureDeviceAddressKHR"));
            vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<decltype(vkGetAccelerationStructureBuildSizesKHR)>(vkGetDeviceProcAddr(*device, "vkGetAccelerationStructureBuildSizesKHR"));
      }

      VkAccelerationStructureBuildSizesInfoKHR AccelerationStructure::query_required_size()
      {
            auto device = get_dependency<Device>();

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo;

            std::vector<uint32_t> primitiveCounts;
            primitiveCounts.reserve(m_rangeInfos.size());
            for (const auto& rangeInfo : m_rangeInfos)
                  primitiveCounts.push_back(rangeInfo.primitiveCount);

            vkGetAccelerationStructureBuildSizesKHR(
                  *device,
                  VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                  &m_buildInfo,
                  primitiveCounts.data(),
                  &sizeInfo
            );

            return sizeInfo;
      }

      DependencyId AccelerationStructure::dependency_id()
      {
            return "AccelerationStructure";
      }

      // --------------------------------------
      // COMPLETE ACCELERATION STRUCTURE
      // --------------------------------------

      void CompleteAccelerationStructure::initialize(
            REF(Device) device,
            REF(CommandPool) commandPool,
            REF(Queue) queue
      )
      {
            add_dependency(device);
            add_dependency(commandPool);
            add_dependency(queue);
      }
      void CompleteAccelerationStructure::add_geometry(
            const std::vector<AccelerationStructureTriangleMesh>& meshes
      )
      {
            auto device = get_dependency<Device>();

            m_lowlevelAccelerationStructures.push_back(AccelerationStructure());
            auto& accelerationStructure = m_lowlevelAccelerationStructures.back();

            accelerationStructure.initialize(device, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

            accelerationStructure.set_triangle_data(meshes);
      }
      void CompleteAccelerationStructure::create()
      {

      }
      void CompleteAccelerationStructure::destroy()
      {

      }

      void CompleteAccelerationStructure::build()
      {
            auto device = get_dependency<Device>();
            auto commandPool = get_dependency<CommandPool>();
            auto queue = get_dependency<Queue>();

            VkCommandBuffer commandBuffer = begin_single_time_cmd_buffer(*commandPool, *device);

            std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos;
            buildInfos.reserve(m_lowlevelAccelerationStructures.size() + 1);

            buildInfos.push_back(m_toplevelAccelerationStructure.get_build_info());
            for (auto& lowlevelAS : m_lowlevelAccelerationStructures)
                  buildInfos.push_back(lowlevelAS.get_build_info());

            std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos;
            rangeInfos.reserve(m_lowlevelAccelerationStructures.size() + 1);
            rangeInfos.push_back(m_toplevelAccelerationStructure.get_range_info());
            for (auto& lowlevelAS : m_lowlevelAccelerationStructures)
                  rangeInfos.push_back(lowlevelAS.get_range_info());

            vkCmdBuildAccelerationStructuresKHR(
                  commandBuffer,
                  static_cast<uint32_t>(buildInfos.size()),
                  buildInfos.data(),
                  rangeInfos.data()
            );

            end_single_time_cmd_buffer(commandBuffer, *commandPool, *device, *queue);
      }

      void CompleteAccelerationStructure::set_vulkan_function_pointers()
      {
            auto device = get_dependency<Device>();
            vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<decltype(vkCmdBuildAccelerationStructuresKHR)>(vkGetDeviceProcAddr(*device, "vkCmdBuildAccelerationStructuresKHR"));
      }

      DependencyId CompleteAccelerationStructure::dependency_id()
      {
            return "CompleteAccelerationStructure";
      }

} // namespace vk