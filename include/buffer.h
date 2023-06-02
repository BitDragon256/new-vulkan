#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "vulkan_helpers.h"

template<class T>
class Buffer
{
public:
    NVE_RESULT create(const std::vector<T>& data, VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags flags, VkBufferUsageFlags usage)
    {
        set(data);
        create(device, flags, usage);
    }
	NVE_RESULT create(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags flags, VkBufferUsageFlags usage)
	{
        m_device = device;
        m_physicalDevice = physicalDevice;
        m_memoryFlags = flags;
        m_usage = usage;

        if (m_data.size() == 0)
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
        allocInfo.memoryTypeIndex = find_memory_type(m_physicalDevice, memRequirements.memoryTypeBits, flags);

        {
            auto res = vkAllocateMemory(m_device, &allocInfo, nullptr, &m_memory);
            log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_device, m_buffer, m_memory, 0);

        reload_data();

        m_created = true;
	}
    NVE_RESULT reload_data()
    {
        void* data;
        vkMapMemory(m_device, m_memory, 0, m_realSize, 0, &data);
        memcpy(data, m_data.data(), (size_t) m_realSize);
        vkUnmapMemory(m_device, m_memory);
    }
    NVE_RESULT set(const std::vector<T>& data)
    {
        bool reload = data.size() != m_data.size();
        m_data = data;

        if (reload)
            reload_data();
    }
    NVE_RESULT destroy()
    {
        if (m_created)
        {
            vkDestroyBuffer(m_device, m_buffer, nullptr);
            vkFreeMemory(m_device, m_memory, nullptr);
        }
    }

private:
	VkBuffer m_buffer;
	VkDeviceMemory m_memory;
	bool m_created;
    VkDeviceSize m_realSize;

	VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkMemoryPropertyFlags m_memoryFlags;
    VkBufferUsageFlags m_usage;

	std::vector<T> m_data;
};