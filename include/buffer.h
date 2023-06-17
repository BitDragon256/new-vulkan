#pragma once

#include <cstring>
#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "vulkan_helpers.h"
#include "logger.h"

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkMemoryPropertyFlags memoryFlags;
    VkBufferUsageFlags usage;

    // staged buffer
    bool useStagedBuffer;
    VkCommandPool stagedBufferTransferCommandPool;
    VkQueue stagedBufferTransferQueue;
} BufferConfig;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool transferCommandPool;
    VkQueue transferQueue;
} StagingBufferConfig;

template<class T>
class StagingBuffer
{
public:
    StagingBuffer() :
        m_created{ false }, m_initialized{ false }, m_data(), m_realSize{ 0 } {}

    void initialize(StagingBufferConfig config)
    {
        m_config = config;
        m_initialized = true;
    }
    NVE_RESULT create(const std::vector<T>& data, StagingBufferConfig config)
    {
        set(data);
        initialize(config);
        recreate();
        reload_data();

        return NVE_SUCCESS;
    }
    NVE_RESULT recreate()
    {
        if (m_data.size() == 0 || !m_initialized)
            return NVE_FAILURE;

        destroy();

        m_realSize = sizeof(T) * m_data.size();

        create_buffer(m_realSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_buffer, m_memory);

        m_created = true;

        return NVE_SUCCESS;
    }
    NVE_RESULT reload_data()
    {
        if (!m_created)
            return NVE_FAILURE;

        void* data;
        vkMapMemory(m_config.device, m_memory, 0, m_realSize, 0, &data);
        std::memcpy(data, m_data.data(), (size_t)m_realSize);
        vkUnmapMemory(m_config.device, m_memory);

        return NVE_SUCCESS;
    }
    NVE_RESULT set(const std::vector<T>& data)
    {
        bool reload = data.size() != m_data.size();
        m_data = data;

        if (reload)
        {
            recreate();
            reload_data();
        }

        return NVE_SUCCESS;
    }
    NVE_RESULT destroy()
    {
        if (m_created)
        {
            vkDestroyBuffer(m_config.device, m_buffer, nullptr);
            vkFreeMemory(m_config.device, m_memory, nullptr);
        }

        return NVE_SUCCESS;
    }

    VkBuffer m_buffer;
private:
    bool m_created;
    bool m_initialized;

    VkDeviceMemory m_memory;
    VkDeviceSize m_realSize;

    StagingBufferConfig m_config;

    std::vector<T> m_data;

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, VkBuffer& buffer, VkDeviceMemory& memory)
    {
        // create buffer

        VkBufferCreateInfo bufferCI = {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = size;
        bufferCI.usage = usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            auto res = vkCreateBuffer(m_config.device, &bufferCI, nullptr, &buffer);
            log_cond_err(res == VK_SUCCESS, "failed to create buffer");
        }

        // allocate buffer

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_config.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(m_config.physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

        {
            auto res = vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_memory);
            log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_config.device, buffer, memory, 0);
    }
};

template<class T>
class Buffer
{
public:
    Buffer() :
        m_created{ false }, m_initialized{ false }, m_data(), m_realSize{ 0 }  {}
    ~Buffer()
    {
        destroy();
    }

    void initialize(BufferConfig config)
    {
        m_config = config;
        m_initialized = true;

        if (m_config.useStagedBuffer)
        {
            m_config.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            m_stagingBufferConfig = {};
            m_stagingBufferConfig.device = m_config.device;
            m_stagingBufferConfig.physicalDevice = m_config.physicalDevice;
            m_stagingBufferConfig.transferCommandPool = m_config.stagedBufferTransferCommandPool;
            m_stagingBufferConfig.transferQueue = m_config.stagedBufferTransferQueue;
        }
    }
    NVE_RESULT create(const std::vector<T>& data, BufferConfig config)
    {
        set(data);
        initialize(config);
        recreate();
        reload_data();

        return NVE_SUCCESS;
    }
	NVE_RESULT create(BufferConfig config)
	{
        initialize(config);
        recreate();

        return NVE_SUCCESS;
	}
    NVE_RESULT recreate()
    {
        if (m_data.size() == 0 || !m_initialized)
            return NVE_FAILURE;

        destroy();

        m_realSize = sizeof(T) * m_data.size();

        create_buffer(m_realSize, m_config.usage, m_config.memoryFlags, m_buffer, m_memory);

        m_created = true;

        return NVE_SUCCESS;
    }
    NVE_RESULT reload_data()
    {
        if (!m_created)
            return NVE_FAILURE;

        if (m_config.useStagedBuffer)
        {
            m_stagingBuffer.create(m_data, m_stagingBufferConfig);
            copy_buffer(m_stagingBuffer.m_buffer, m_buffer, m_realSize);
            m_stagingBuffer.destroy();
        }
        else
        {
            void* data;
            vkMapMemory(m_config.device, m_memory, 0, m_realSize, 0, &data);
            memcpy(data, m_data.data(), (size_t)m_realSize);
            vkUnmapMemory(m_config.device, m_memory);
        }

        return NVE_SUCCESS;
    }
    NVE_RESULT set(const std::vector<T>& data)
    {
        bool reload = data.size() != m_data.size();
        m_data = data;

        if (reload)
        {
            recreate();
            reload_data();
        }

        return NVE_SUCCESS;
    }
    NVE_RESULT destroy()
    {
        if (m_created)
        {
            vkDestroyBuffer(m_config.device, m_buffer, nullptr);
            vkFreeMemory(m_config.device, m_memory, nullptr);

            m_created = false;
        }

        return NVE_SUCCESS;
    }

	VkBuffer m_buffer;
private:
	bool m_created;
    bool m_initialized;

	VkDeviceMemory m_memory;
    VkDeviceSize m_realSize;

    BufferConfig m_config;

	std::vector<T> m_data;

    StagingBufferConfig m_stagingBufferConfig;
    StagingBuffer<T> m_stagingBuffer;

    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags, VkBuffer& buffer, VkDeviceMemory& memory)
    {
        // create buffer

        VkBufferCreateInfo bufferCI = {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = size;
        bufferCI.usage = usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            auto res = vkCreateBuffer(m_config.device, &bufferCI, nullptr, &buffer);
            log_cond_err(res == VK_SUCCESS, "failed to create buffer");
        }

        // allocate buffer

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_config.device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(m_config.physicalDevice, memRequirements.memoryTypeBits, memoryFlags);

        {
            auto res = vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_memory);
            log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_config.device, buffer, memory, 0);
    }
    void copy_buffer(VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size)
    {
        VkCommandBuffer cmdBuffer = begin_single_time_cmd_buffer(m_config.stagedBufferTransferCommandPool, m_config.device);
        VkBufferCopy copyRegion = {};
        copyRegion.size = m_realSize;
        vkCmdCopyBuffer(cmdBuffer, srcBuf, dstBuf, 1, &copyRegion);
        end_single_time_cmd_buffer(cmdBuffer, m_config.stagedBufferTransferCommandPool, m_config.device, m_config.stagedBufferTransferQueue);
    }
};