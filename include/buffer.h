#pragma once

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
    bool useStagedBuffer;
} BufferConfig;

template<class T>
class Buffer
{
public:
    Buffer() :
        m_created{ false }, m_initialized{ false }, m_data(), m_realSize{ 0 }  {}

    void initialize(BufferConfig config)
    {
        m_config = config;
        m_initialized = true;
    }
    NVE_RESULT create(const std::vector<T>& data, BufferConfig config)
    {
        set(data);
        initialize(config);
        recreate();
        reload_data();

        return NVE_SUCCESS;
    }
	NVE_RESULT create(VkDevice device, BufferConfig config)
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

        VkBufferCreateInfo bufferCI = {};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = m_realSize;
        bufferCI.usage = m_config.usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            auto res = vkCreateBuffer(m_config.device, &bufferCI, nullptr, &m_buffer);
            log_cond_err(res == VK_SUCCESS, "failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_config.device, m_buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(m_config.physicalDevice, memRequirements.memoryTypeBits, m_config.memoryFlags);

        {
            auto res = vkAllocateMemory(m_config.device, &allocInfo, nullptr, &m_memory);
            log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_config.device, m_buffer, m_memory, 0);

        m_created = true;

        return NVE_SUCCESS;
    }
    NVE_RESULT reload_data()
    {
        if (!m_created)
            return NVE_FAILURE;

        void* data;
        vkMapMemory(m_config.device, m_memory, 0, m_realSize, 0, &data);
        memcpy(data, m_data.data(), (size_t) m_realSize);
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

    BufferConfig m_config;

	std::vector<T> m_data;
};