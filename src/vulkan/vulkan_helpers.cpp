#include "vulkan/vulkan_helpers.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "logger.h"

#include "vulkan/vulkan_handles.h"

bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    bool suitable{ 1 };

    auto indices = find_queue_families(device, surface);
    suitable &= indices.is_complete();

    bool extensionsSupported = check_device_extension_support(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    suitable &= swapChainAdequate;

    // the physical m_device needs to support geometry shaders and be a discrete gpu
    return suitable;
}
uint32_t rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // query for the m_device's properties and features
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    if (!is_device_suitable(device, surface))
        return 0;
        
    uint32_t score{ 0 };
    
    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }
    
    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}
bool check_device_extension_support(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}
QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

    uint32_t queueFamilyIndex{ 0 };
    for (const auto& queueFamilyProperty : queueFamilyProperties)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, queueFamilyIndex, surface, &presentSupport);

        if (presentSupport)
            indices.presentationFamily = queueFamilyIndex;

        if (queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = queueFamilyIndex;

        if (queueFamilyProperty.queueFlags & VK_QUEUE_TRANSFER_BIT)
            indices.transferFamily = queueFamilyIndex;

        if (queueFamilyProperty.queueFlags & VK_QUEUE_COMPUTE_BIT)
            indices.computeFamily = queueFamilyIndex;


        if (indices.is_complete())
            break;

        queueFamilyIndex++;
    }

    return indices;
}

bool QueueFamilyIndices::is_complete()
{
    return
        graphicsFamily.has_value() &&
        presentationFamily.has_value() &&
        transferFamily.has_value() &&
        computeFamily.has_value();
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    uint32_t formatCount{ 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount{ 0 };
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}
VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

std::vector<char> read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    logger::log_cond_err(file.is_open(), "failed to open file");

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
VkShaderModule create_shader_module(const std::string& file, VkDevice device)
{
    std::string prefix = ROOT_DIRECTORY;

    std::vector<char> shaderCode = read_file(prefix + "/shaders/" + file);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    VkShaderModule shaderModule;
    auto res = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    logger::log_cond_err(res == VK_SUCCESS, "failed to create shader module");

    return shaderModule;
}

uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    logger::log_err("failed to find suitable memory type");
    return 0;
}

VkCommandBuffer begin_single_time_cmd_buffer(VkCommandPool cmdPool, VkDevice device)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}
void end_single_time_cmd_buffer(VkCommandBuffer commandBuffer, VkCommandPool cmdPool, VkDevice device, VkQueue queue)
{
    auto res = vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFence fence;
    VkFenceCreateInfo fenceCI = {};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.pNext = nullptr;
    fenceCI.flags = 0;

    vkCreateFence(device, &fenceCI, nullptr, &fence);

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device, fence, nullptr);

    vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}
void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
}
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    logger::log("validation layer: " + std::string(pCallbackData->pMessage));

    return VK_FALSE;
}
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    logger::log_err("failed to find supported format");
    return (VkFormat) 0;
}
VkFormat find_depth_format(VkPhysicalDevice physicalDevice) {
    return find_supported_format(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
        physicalDevice
    );
}


/*
* --------------------------------
*          OBJECT CREATION
* --------------------------------
 */
VkSemaphore vk_create_semaphore(VkDevice device)
{
    VkSemaphoreCreateInfo semCI = {};
    semCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semCI.pNext = nullptr;
    semCI.flags = 0;
    
    VkSemaphore sem;
    vkCreateSemaphore(device, &semCI, nullptr, &sem);
    return sem;
}
void vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore)
{
    vkDestroySemaphore(device, semaphore, nullptr);
}

VkBufferCreateInfo make_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
{
      VkBufferCreateInfo bufferCI = {};
      bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      bufferCI.pNext = nullptr;
      bufferCI.flags = 0;
      bufferCI.size = size;
      bufferCI.usage = usage;
      bufferCI.sharingMode = sharingMode;

      return bufferCI;
}
VkBuffer make_buffer(REF(vk::Device) device, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode)
{
      auto bufferCI = make_buffer_create_info(size, usage, sharingMode);
      VkBuffer buffer;
      VK_CHECK_ERROR( vkCreateBuffer(*device, &bufferCI, nullptr, &buffer) )
      return buffer;
}
void destroy_buffer(REF(vk::Device) device, VkBuffer buffer)
{
      vkDestroyBuffer(*device, buffer, nullptr);
}
VkDeviceMemory allocate_buffer_memory(REF(vk::Device) device, VkBuffer buffer, VkMemoryPropertyFlags propertyFlags)
{
      VkMemoryRequirements memoryRequirements;
      vkGetBufferMemoryRequirements(*device, buffer, &memoryRequirements);

      VkMemoryAllocateInfo memoryAI = {};
      memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      memoryAI.pNext = nullptr;
      memoryAI.allocationSize = memoryRequirements.size;
      memoryAI.memoryTypeIndex = device->memory_type_index(memoryRequirements.memoryTypeBits, propertyFlags);

      VkDeviceMemory memory;
      VK_CHECK_ERROR( vkAllocateMemory(*device, &memoryAI, nullptr, &memory) )

      return memory;
}
VkDeviceAddress get_buffer_device_address(REF(vk::Device) device, VkBuffer buffer)
{
      VkBufferDeviceAddressInfo bufferDeviceAddressInfo = {};
      bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
      bufferDeviceAddressInfo.buffer = buffer;
      return vkGetBufferDeviceAddress(*device, &bufferDeviceAddressInfo);
}

void free_device_memory(REF(vk::Device) device, VkDeviceMemory memory)
{
      vkFreeMemory(*device, memory, nullptr);
}

void* get_pnext(void* structure)
{
      return to_base_structure(structure)->pNext;
}
bool pnext_is_null(void* structure)
{
      return get_pnext(structure) == nullptr;
}
VkStructureType get_structure_type(void* structure)
{
      return to_base_structure(structure)->sType;
}
VkBaseOutStructure* to_base_structure(void* structure)
{
      return reinterpret_cast<VkBaseOutStructure*>(structure);
}

uint32_t aligned_size(uint32_t value, uint32_t alignment)
{
      return (value + alignment - 1) & ~(alignment - 1);
}
