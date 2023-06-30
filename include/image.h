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
		VkMemoryPropertyFlags memoryProperties,
		VkImageAspectFlags aspect
	);
	void destroy();

	VkImage m_image;
	VkImageView m_imageView;
	VkDeviceMemory m_memory;

private:
	VkDevice m_device;
};