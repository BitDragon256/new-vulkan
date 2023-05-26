#pragma once

#include <vulkan/vulkan.hpp>

#include "models.h"

typedef int NVE_RESULT;
#define NVE_SUCCESS 0
#define NVE_FAILURE -1

class Renderer
{
public:
	NVE_RESULT render();
	NVE_RESULT init();
	NVE_RESULT bind_model_handler(ModelHandler* handler);

private:
	// vulkan objects
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	
	// vulkan object creation
	NVE_RESULT create_instance();
	NVE_RESULT get_physical_device();
	NVE_RESULT create_logical_device();
};
