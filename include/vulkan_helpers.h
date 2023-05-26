#pragma once

#include <optional>

#include <vulkan/vulkan.h>

// physical m_device
bool is_device_suitable(VkPhysicalDevice device);
uint32_t rate_device_suitability(VkPhysicalDevice device);

//queues
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentationFamily;
	bool is_complete();
};
QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);