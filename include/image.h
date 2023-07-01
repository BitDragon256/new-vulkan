#pragma once

#include <vulkan/vulkan.h>

class VImage
{
public:
	void create(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	);
	void destroy();

	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_memory;

private:
	VkDevice m_device;
};