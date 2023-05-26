#include "render.h"

#include <vector>
#include <map>

#include "logger.h"
#include "vulkan_helpers.h"

#define RATE_PHYSICAL_DEVICES

NVE_RESULT Renderer::init()
{
    NVE_RESULT res;
    
    res = create_instance();
    res = get_physical_device();
    
    return NVE_SUCCESS;
}

NVE_RESULT Renderer::create_instance()
{
    VkInstanceCreateInfo instanceCI = {};
    instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    
    VkResult res = vkCreateInstance(&instanceCI, nullptr, &instance);
    log_cond_err(res == VK_SUCCESS, "instance creation failed");
    log("instance created");
    
    return NVE_SUCCESS;
}

NVE_RESULT Renderer::get_physical_device()
{
    // get all available physical devices
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    log_cond_err(deviceCount != 0, "no valid physical devices found");
    std::vector<VkPhysicalDevice> availableDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());
    
#ifdef RATE_PHYSICAL_DEVICES

    std::multimap<uint32_t, VkPhysicalDevice> ratedDevices;
    for (const auto& device : availableDevices)
    {
        uint32_t score = rate_device_suitability(device);
        ratedDevices.insert(std::make_pair(score, device));
    }

    log_cond_err(ratedDevices.rbegin()->first > 0, "no acceptable physical device found");

    physicalDevice = ratedDevices.rbegin()->second;

#else
    
    physicalDevice = VK_NULL_HANDLE;
    for (const auto& device : availableDevices) 
    {
        if (is_device_suitable(device))
        {
            physicalDevice = device;
            break;
        }
    }
    log_cond_err(physicalDevice != VK_NULL_HANDLE, "no acceptable physical device found");
    
#endif
    
    log("physical device found");
    return NVE_SUCCESS;
}