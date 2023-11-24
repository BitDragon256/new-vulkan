#pragma once

#include <cstring>

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "vulkan_helpers.h"
#include "logger.h"
#include "profiler.h"

#define BUFFER_PROFILER
#ifdef BUFFER_PROFILER 
#define PROFILE_START(X) m_profiler.start_measure(X);
#define PROFILE_END(X) m_profiler.end_measure(X, true);
#define PROFILE_LABEL(X) m_profiler.begin_label(X);
#define PROFILE_LABEL_END m_profiler.end_label();
#else
#define PROFILE_START(X)
#define PROFILE_END(X) 0.f;
#define PROFILE_LABEL(X)
#define PROFILE_LABEL_END
#endif

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkMemoryPropertyFlags memoryFlags;
    VkBufferUsageFlags usage;

    // staged buffer
    bool useStagedBuffer;
    bool singleUseStagedBuffer;
    VkCommandPool stagedBufferTransferCommandPool;
    VkQueue stagedBufferTransferQueue;
    bool selfSubmitStagedBufferCopy;
} BufferConfig;

typedef struct {
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkMemoryPropertyFlags memoryFlags;
    VkBufferUsageFlags usage;
} RawBufferConfig;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDevice device;
} BufferDestructionInfo;

inline bool operator==(const BufferDestructionInfo& a, const BufferDestructionInfo& b)
{
    return a.buffer == b.buffer && a.memory == b.memory && a.device == b.device;
}

static std::vector<BufferDestructionInfo> AllBuffers;
inline void destroy_all_corresponding_buffers(VkDevice device)
{
    for (auto& buffer : AllBuffers)
    {
        if (buffer.device == device)
        {
            vkDestroyBuffer(buffer.device, buffer.buffer, nullptr);
            vkFreeMemory(buffer.device, buffer.memory, nullptr);
        }
    }
}

inline bool memequal(void* a, void* b, size_t stride)
{
    if (stride % 4 != 0)
    {
        std::cerr << "stride must be multiple of four!\n";
        abort();
    }
    for (size_t i = 0; i < stride; i += 4)
        if (*((int*)a + i) != *((int*)b + i))
            return false;
    return true;
}

template<class T>
class RawBuffer
{
public:
    RawBuffer() :
        m_created{ false }, m_initialized{ false }, m_data(), m_realSize{ 0 } {}

    // sets the data of the buffer and returns false when the data is not new
    bool set(const std::vector<T>& data)
    {
        if (!m_initialized || data.size() == 0)
            return false;

        bool recreate = data.size() > m_data.size() || !m_created;
        m_data = data;
        size_t tSize = sizeof(T);
        m_realSize = tSize * m_data.size();

        std::string typeName = typeid(*this).name();
        PROFILE_START("recreate buffer " + typeName);
        if (recreate)
            create();
        PROFILE_END("recreate buffer" + typeName);
        PROFILE_START("check reload");
        bool reload = recreate;
        for (size_t i = 0; i < data.size() && !reload; i++)
            if (!memequal((void*) & m_data[i], (void*) & data[i], tSize))
                reload = true;
        PROFILE_END("check reload");
        if (reload)
        {
            PROFILE_LABEL("reload data " + typeName);
            reload_data();
            PROFILE_LABEL_END
        }
        else
            return false;
        return true;
    }
    void cpy_raw(uint32_t size, const T* data)
    {
        m_realSize = sizeof(T) * size;
        create();
        cpy_data(data);
    }
    void destroy()
    {
        if (m_created)
        {
            vkDestroyBuffer(m_config.device, m_buffer, nullptr);
            vkFreeMemory(m_config.device, m_memory, nullptr);

            m_created = false;
            std::erase(AllBuffers, destruction_info());
        }
    }
    VkDeviceSize range()
    {
        return m_realSize;
    }

    VkBuffer m_buffer;

protected:
    void initialize(RawBufferConfig config)
    {
        m_config = config;
        m_initialized = true;

        set({ T() }); // one dummy element
    }
    void create()
    {
        vkDeviceWaitIdle(m_config.device); // TODO better syncing

        destroy();

        create_buffer(m_realSize, m_config.usage, m_config.memoryFlags, m_buffer, m_memory);

        m_created = true;
        AllBuffers.push_back(destruction_info());
    }
    virtual void reload_data()
    {
        cpy_data(m_data.data());
    }
    void cpy_data(const T* d)
    {
        void* data;
        PROFILE_START("map mem");
        vkMapMemory(m_config.device, m_memory, 0, m_realSize, 0, &data);
        PROFILE_END("map mem");
        PROFILE_START("cpy mem");
        std::memcpy(data, d, (size_t)m_realSize);
        PROFILE_END("cpy mem");
        PROFILE_START("unmap mem");
        vkUnmapMemory(m_config.device, m_memory);
        PROFILE_END("unmap mem");
    }

    bool m_created;
    bool m_initialized;

    RawBufferConfig m_config;

    VkDeviceMemory m_memory;
    VkDeviceSize m_realSize;

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
            logger::log_cond_err(res == VK_SUCCESS, "failed to create buffer");
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
            logger::log_cond_err(res == VK_SUCCESS, "failed to allocate buffer memory");
        }

        vkBindBufferMemory(m_config.device, buffer, memory, 0);
    }

    Profiler m_profiler;

private:

    BufferDestructionInfo destruction_info()
    {
        return BufferDestructionInfo{ m_buffer, m_memory, m_config.device };
    }

};

template<class T>
class StagingBuffer : public RawBuffer<T>
{
public:
    void initialize(VkDevice device, VkPhysicalDevice physicalDevice)
    {
        RawBufferConfig config = {
            device,
            physicalDevice,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        };

        RawBuffer<T>::initialize(config);
    }
};

template<class T>
class Buffer : public RawBuffer<T>
{
public:
    void initialize(BufferConfig bufferConfig)
    {
        m_config = bufferConfig;

        if (m_config.useStagedBuffer)
        {
            m_config.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            m_config.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            VkCommandBufferAllocateInfo cmdBufAI;
            cmdBufAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdBufAI.pNext = 0;
            cmdBufAI.commandBufferCount = 1;
            cmdBufAI.commandPool = m_config.stagedBufferTransferCommandPool;
            if (m_config.selfSubmitStagedBufferCopy)
                cmdBufAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            else
                cmdBufAI.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
            vkAllocateCommandBuffers(m_config.device, &cmdBufAI, &m_commandBuffer);
        }

        m_stagingBuffer.initialize(m_config.device, m_config.physicalDevice);

        RawBufferConfig config = {
            m_config.device,
            m_config.physicalDevice,
            m_config.memoryFlags,
            m_config.usage
        };

        RawBuffer<T>::initialize(config);
    }
    void destroy()
    {
        m_stagingBuffer.destroy();
        RawBuffer<T>::destroy();
    }
    VkCommandBuffer staged_cpy_cmd_buf()
    {
        return m_commandBuffer;
    }
protected:
    void reload_data() override
    {
        if (m_config.useStagedBuffer)
        {
            m_stagingBuffer.set(RawBuffer<T>::m_data);
            copy_buffer(m_stagingBuffer.m_buffer, RawBuffer<T>::m_buffer, RawBuffer<T>::m_realSize);
            if (m_config.singleUseStagedBuffer)
                m_stagingBuffer.destroy();
        }
        else
        {
            RawBuffer<T>::reload_data();
        }
    }

    BufferConfig m_config;
    StagingBuffer<T> m_stagingBuffer;

    void copy_buffer(VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size)
    {
        PROFILE_START("begin cmd buf");
        vkResetCommandBuffer(m_commandBuffer, 0);
        VkCommandBufferBeginInfo cmdBufBI;
        cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBI.flags = 0;
        cmdBufBI.pNext = 0;
        cmdBufBI.pInheritanceInfo = 0;
        vkBeginCommandBuffer(m_commandBuffer, &cmdBufBI);
        // VkCommandBuffer cmdBuffer = begin_single_time_cmd_buffer(m_config.stagedBufferTransferCommandPool, m_config.device);
        PROFILE_END("begin cmd buf");
        VkBufferCopy copyRegion = {};
        copyRegion.size = RawBuffer<T>::m_realSize;
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        PROFILE_START("record cmd buf");
        vkCmdCopyBuffer(m_commandBuffer, srcBuf, dstBuf, 1, &copyRegion);
        PROFILE_END("record cmd buf");
        PROFILE_START("submit cmd buf");
        // end_single_time_cmd_buffer(cmdBuffer, m_config.stagedBufferTransferCommandPool, m_config.device, m_config.stagedBufferTransferQueue);
        vkEndCommandBuffer(m_commandBuffer);

        if (m_config.selfSubmitStagedBufferCopy)
        {
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_commandBuffer;

            VkFence fence;
            VkFenceCreateInfo fenceCI = {};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCI.pNext = nullptr;
            fenceCI.flags = 0;

            vkCreateFence(m_config.device, &fenceCI, nullptr, &fence);

            res = vkQueueSubmit(m_config.stagedBufferTransferQueue, 1, &submitInfo, fence);
            res = vkWaitForFences(m_config.device, 1, &fence, VK_TRUE, UINT64_MAX);

            vkDestroyFence(m_config.device, fence, nullptr);
        }

        PROFILE_END("submit cmd buf");
    }

private:

    Profiler m_profiler;
    VkCommandBuffer m_commandBuffer;
};

#undef PROFILE_START(X)
#undef PROFILE_END(X)
#undef PROFILE_LABEL(X)
#undef PROFILE_LABEL_END