#pragma once

#include <vulkan/vulkan.h>

bool is_device_suitable(VkPhysicalDevice device);
uint32_t rate_device_suitability(VkPhysicalDevice device);