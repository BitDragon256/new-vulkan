#pragma once

#include <optional>

#include <vulkan/vulkan.h>

// physical device
bool is_device_suitable(VkPhysicalDevice device);
uint32_t rate_device_suitability(VkPhysicalDevice device);

//queues
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	bool is_complete();
};
QueueFamilyIndices find_queue_families(VkPhysicalDevice device);