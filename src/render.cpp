#include "render.h"

#include <vector>
#include <map>
#include <set>

#include "logger.h"
#include "vulkan_helpers.h"

#define RATE_PHYSICAL_DEVICES

NVE_RESULT Renderer::init(RenderConfig config)
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
    
    VkResult res = vkCreateInstance(&instanceCI, nullptr, &m_instance);
    log_cond_err(res == VK_SUCCESS, "instance creation failed");
    log("instance created");
    
    return NVE_SUCCESS;
}

NVE_RESULT Renderer::get_physical_device()
{
    // get all available physical devices
    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    log_cond_err(deviceCount != 0, "no valid physical devices found");
    std::vector<VkPhysicalDevice> availableDevices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, availableDevices.data());
    
#ifdef RATE_PHYSICAL_DEVICES

    std::multimap<uint32_t, VkPhysicalDevice> ratedDevices;
    for (const auto& device : availableDevices)
    {
        uint32_t score = rate_device_suitability(device);
        ratedDevices.insert(std::make_pair(score, device));
    }

    log_cond_err(ratedDevices.rbegin()->first > 0, "no acceptable physical device found");

    m_physicalDevice = ratedDevices.rbegin()->second;

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

NVE_RESULT Renderer::create_device()
{
    QueueFamilyIndices queueFamilyIndices = find_queue_families(m_physicalDevice, m_surface);

    // specify queues
    std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs = {};
    std::set<uint32_t> uniqueQueueFamilyIndices = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentationFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        VkDeviceQueueCreateInfo deviceQueueCI = {};

        deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCI.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCI.queueCount = 1;
        deviceQueueCI.pQueuePriorities = &queuePriority;
        deviceQueueCIs.push_back(deviceQueueCI);
    }

    // physical device stuff
    VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

    // m_device creation
    VkDeviceCreateInfo deviceCI = {};
    deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCI.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCIs.size());
    deviceCI.pQueueCreateInfos = deviceQueueCIs.data();
    deviceCI.pEnabledFeatures = &physicalDeviceFeatures;

    // TODO validation layers
    deviceCI.enabledExtensionCount = 0;

    auto res = vkCreateDevice(m_physicalDevice, &deviceCI, nullptr, &m_device);
    log_cond_err(res == VK_SUCCESS, "physical device creation failed");
    log("physical device created");

    vkGetDeviceQueue(m_device, queueFamilyIndices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, queueFamilyIndices.presentationFamily.value(), 0, &m_presentationQueue);

    return NVE_SUCCESS;
}
NVE_RESULT Renderer::create_window(int width, int height, std::string title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
}
NVE_RESULT Renderer::get_surface()
{
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
}
NVE_RESULT Renderer::create_swapchain(int height, int width)
{
    
}
NVE_RESULT Renderer::create_image_views()
{

}
NVE_RESULT Renderer::create_pipeline()
{

}

NVE_RESULT Renderer::render()
{
    if (glfwWindowShouldClose(m_window))
    {
        clean_up();
        return;
    }

    glfwPollEvents();
}
void Renderer::clean_up()
{
    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    glfwDestroyWindow(m_window);
    glfwTerminate();
}