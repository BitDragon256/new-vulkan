#include "render.h"

#include <vector>
#include <map>

#include "logger.h"
#include "vulkan_helpers.h"

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
    assert(res == VK_SUCCESS && "instance creation failed");
    log("instance created");
    
    return NVE_SUCCESS;
}

NVE_RESULT Renderer::get_physical_device()
{
    // get all available physical devices
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    assert(deviceCount != 0 && "no valid physical devices found");
    std::vector<VkPhysicalDevice> availableDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());
    
#ifdef RATE_PHYSICAL_DEVICES

    

#else
    
    for (const auto& device : availableDevices) 
    {
        if (is_device_suitable(device))
        {
            physicalDevice = device;
            break;
        }
    }
    
#endif
    
    assert(physicalDevice != VK_NULL_HANDLE && "no acceptable physical device found");
    
    log("physical device found");
    return NVE_SUCCESS;
}