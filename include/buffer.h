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
    for (size_t i = 0; i < stride; i += 1)
    {
        if (*((uint8_t*)a + i) != *((uint8_t*)b + i))
        {
            return false;
        }
    }
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
        m_realSize = sizeof(T) * data.size();

        std::string typeName = typeid(*this).name();
        if (recreate)
        {
            PROFILE_START("recreate buffer " + typeName);
            create();
            PROFILE_END("recreate buffer " + typeName);
        }
        bool reload = recreate;
        for (size_t i = 0; i < data.size() && !reload; i++)
        {
            // if (!memequal((void*)&m_data[i], (void*)&data[i], tSize))
            if (!(m_data[i] == data[i]))
            {
                reload = true;
                break;
            }
        }
        m_data = data;
        if (reload)
        {
            // PROFILE_LABEL("reload data " + typeName);
            reload_data();
            // PROFILE_LABEL_END
        }
        return reload;
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
        // PROFILE_START("map mem");
        vkMapMemory(m_config.device, m_memory, 0, m_realSize, 0, &data);
        // PROFILE_END("map mem");
        // PROFILE_START("cpy mem");
        std::memcpy(data, d, (size_t)m_realSize);
        // PROFILE_END("cpy mem");
        // PROFILE_START("unmap mem");
        vkUnmapMemory(m_config.device, m_memory);
        // PROFILE_END("unmap mem");
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
            cmdBufAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            vkAllocateCommandBuffers(m_config.device, &cmdBufAI, &m_commandBuffer);

            m_stagedBufCpySemaphoreSignaled = true;
            m_stagedBufCpySubmitted = true;

			VkSemaphoreCreateInfo semCI = {};
			semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			semCI.pNext = nullptr;
			semCI.flags = 0;
			vkCreateSemaphore(m_config.device, &semCI, nullptr, &m_stagedBufCpySemaphore);

            VkFenceCreateInfo fenceCI = {};
            fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCI.pNext = nullptr;
            fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            vkCreateFence(m_config.device, &fenceCI, nullptr, &m_lastStagedBufCpyFence);
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
        if (m_config.useStagedBuffer)
        {
            vkDestroySemaphore(m_config.device, m_stagedBufCpySemaphore, nullptr);
            vkDestroyFence(m_config.device, m_lastStagedBufCpyFence, nullptr);
            vkFreeCommandBuffers(m_config.device, m_config.stagedBufferTransferCommandPool, 1, &m_commandBuffer);
        }
        m_stagingBuffer.destroy();
        RawBuffer<T>::destroy();
    }

    VkSemaphore staged_buf_cpy_semaphore()
    {
        if (!m_stagedBufCpySubmitted)
            return VK_NULL_HANDLE;
        m_stagedBufCpySubmitted = false;
        m_stagedBufCpySemaphoreSignaled = true;
        return m_stagedBufCpySemaphore;
    }
    VkFence staged_buf_cpy_fence()
    {
        return m_lastStagedBufCpyFence;
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
        record_cpy_cmd_buf(srcBuf, dstBuf, size);

        // TODO staged buffer copy synchronization
        //if (!m_stagedBufCpySemaphoreSignaled)
        //{
        //    m_stagedBufCpySemaphores.push_back(m_stagedBufCpySemaphore);
        //    m_stagedBufCpySemaphore = vk_create_semaphore(m_config.device);
        //}

		submit_cpy_cmd_buf();
    }

private:

    Profiler m_profiler;
    VkCommandBuffer m_commandBuffer;
    VkSemaphore m_stagedBufCpySemaphore;
    VkFence m_lastStagedBufCpyFence;
    bool m_stagedBufCpySemaphoreSignaled;
    bool m_stagedBufCpySubmitted;

    std::vector<VkSemaphore> m_stagedBufCpySemaphores;

    void record_cpy_cmd_buf(VkBuffer srcBuf, VkBuffer dstBuf, VkDeviceSize size)
    {
        VkResult res;
        // PROFILE_START("begin cmd buf");
        res = vkResetCommandBuffer(m_commandBuffer, 0);
        VkCommandBufferBeginInfo cmdBufBI = {};
        cmdBufBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBufBI.flags = 0;
        cmdBufBI.pNext = 0;
        cmdBufBI.pInheritanceInfo = 0;
        res = vkBeginCommandBuffer(m_commandBuffer, &cmdBufBI);
        // VkCommandBuffer cmdBuffer = begin_single_time_cmd_buffer(m_config.stagedBufferTransferCommandPool, m_config.device);
        // PROFILE_END("begin cmd buf");
        VkBufferCopy copyRegion = {};
        copyRegion.size = RawBuffer<T>::m_realSize;
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        // PROFILE_START("record cmd buf");
        vkCmdCopyBuffer(m_commandBuffer, srcBuf, dstBuf, 1, &copyRegion);
        // vkCmdCopyBuffer(cmdBuffer, srcBuf, dstBuf, 1, &copyRegion);
        // PROFILE_END("record cmd buf");
        // end_single_time_cmd_buffer(cmdBuffer, m_config.stagedBufferTransferCommandPool, m_config.device, m_config.stagedBufferTransferQueue);
        // PROFILE_START("end cmd buf");
        res = vkEndCommandBuffer(m_commandBuffer);
        // PROFILE_END("end cmd buf");
    }
    void submit_cpy_cmd_buf()
    {
        VkResult res;

		// PROFILE_START("wait on last fence");
		// VkResult res = vkWaitForFences(m_config.device, 1, &m_lastStagedBufCpyFence, VK_TRUE, UINT32_MAX);
		vkResetFences(m_config.device, 1, &m_lastStagedBufCpyFence);
		// PROFILE_END("wait on last fence");

		PROFILE_START("submit cmd buf");
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
        // submitInfo.pSignalSemaphores = &m_stagedBufCpySemaphore;

		// submitInfo.waitSemaphoreCount = static_cast<uint32_t>(m_stagedBufCpySemaphores.size());
        // submitInfo.pWaitSemaphores = m_stagedBufCpySemaphores.data();
        // std::vector<VkPipelineStageFlags> waitStages(m_stagedBufCpySemaphores.size(), VK_PIPELINE_STAGE_VERTEX_INPUT_BIT);
        // submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.waitSemaphoreCount = 0;

		res = vkQueueSubmit(m_config.stagedBufferTransferQueue, 1, &submitInfo, m_lastStagedBufCpyFence);
		// res = vkQueueSubmit(m_config.stagedBufferTransferQueue, 1, &submitInfo, VK_NULL_HANDLE);
		PROFILE_END("submit cmd buf");

        PROFILE_START("wait fences");
		res = vkWaitForFences(m_config.device, 1, &m_lastStagedBufCpyFence, VK_TRUE, UINT32_MAX);
        PROFILE_END("wait fences");

        m_stagedBufCpySemaphores.clear();
		m_stagedBufCpySemaphoreSignaled = false;
        m_stagedBufCpySubmitted = true;
    }
    void self_submit_cpy_cmd_buf()
    {
		PROFILE_START("wait on last fence");
		vkWaitForFences(m_config.device, 1, &m_lastStagedBufCpyFence, VK_TRUE, UINT32_MAX);
		vkResetFences(m_config.device, 1, &m_lastStagedBufCpyFence);
		PROFILE_END("wait on last fence");

		PROFILE_START("submit cmd buf");
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer;
		submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr; // &m_stagedBufCpySemaphore;
        std::cout << m_stagedBufCpySemaphore << "\n";
		submitInfo.waitSemaphoreCount = 0;

		VkFence fence;
		VkFenceCreateInfo fenceCI = {};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.pNext = nullptr;
		fenceCI.flags = 0;

		vkCreateFence(m_config.device, &fenceCI, nullptr, &fence);

		auto res = vkQueueSubmit(m_config.stagedBufferTransferQueue, 1, &submitInfo, fence);

		vkWaitForFences(m_config.device, 1, &fence, VK_TRUE, UINT32_MAX);
		vkDestroyFence(m_config.device, fence, nullptr);

		PROFILE_END("submit cmd buf");

		m_stagedBufCpySemaphoreSignaled = false;
        m_stagedBufCpySubmitted = true;

        logger::log("self submit cpy buf");
    }

};

#undef PROFILE_START
#undef PROFILE_END
#undef PROFILE_LABEL
#undef PROFILE_LABEL_END