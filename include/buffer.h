#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "vulkan_helpers.h"
#include "logger.h"

template<class T>
class Buffer
{
public:
    Buffer() :
        m_created{ false }, m_initialized{ false }, m_data(), m_realSize{ 0 }  {}

    void initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags flags, VkBufferUsageFlags usage)
    {
        m_device = device;
        m_physicalDevice = physicalDevice;
        m_memoryFlags = flags;
        m_usage = usage;

        m_initialized = true;
    }
    NVE_RESULT create(const std::vector<T>& data, VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags flags, VkBufferUsageFlags usage)
    {
        set(data);
        initialize(device, physicalDevice, flags, usage);
        recreate();
        reload_data();
    }
	NVE_RESULT create(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags flags, VkBufferUsageFlags usage)
	{
        initialize(device, physicalDevice, flags, usage);
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
        bufferCI.usage = m_usage;
        bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            auto res = vkCreateBuffer(m_device, &bufferCI, nullptr, &m_buffer);
            log_cond_err(res == VK_SUCCESS, "failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, m_buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(m_physicalDevice, memRequirements.memoryTypeBits, m_memoryFlags);

        {
            auto res = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
            log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_device, m_buffer, m_memory, 0);

        m_created = true;
    }
    NVE_RESULT reload_data()
    {
        if (!m_created)
            return NVE_FAILURE;

        void* data;
        vkMapMemory(m_device, m_memory, 0, m_realSize, 0, &data);
        memcpy(data, m_data.data(), (size_t) m_realSize);
        vkUnmapMemory(m_device, m_memory);

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
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            vkFreeMemory(m_device, m_memory, nullptr);
        }

        return NVE_SUCCESS;
    }

	VkBuffer m_buffer;
private:
	bool m_created;
    bool m_initialized;

	VkDeviceMemory m_memory;
    VkDeviceSize m_realSize;

	VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkMemoryPropertyFlags m_memoryFlags;
    VkBufferUsageFlags m_usage;

	std::vector<T> m_data;
};