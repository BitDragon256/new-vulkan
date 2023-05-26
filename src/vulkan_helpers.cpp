#include "vulkan_helpers.h"

#include <vector>

bool is_device_suitable(VkPhysicalDevice device)
{
    // query for the device's properties and features
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    bool suitable{ 1 };

    suitable &= deviceFeatures.geometryShader;

    auto indices = find_queue_families(device);
    suitable &= indices.is_complete();

    // the physical device needs to support geometry shaders and be a discrete gpu
    return suitable;
}
uint32_t rate_device_suitability(VkPhysicalDevice device)
{
    // query for the device's properties and features
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    
    if (!is_device_suitable(device))
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
QueueFamilyIndices find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        if (indices.is_complete())
        {
            break;
        }
        i++;
    }

    return indices;
}

bool QueueFamilyIndices::is_complete()
{
    return graphicsFamily.has_value();
}