#include "image.h"

#include "logger.h"
#include "vulkan_helpers.h"

void VImage::create(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties,
	VkImageAspectFlags aspect)
{
	m_device = device;

	VkExtent3D extent;
	extent.width = width;
	extent.height = height;
	extent.depth = 1;

	VkImageCreateInfo imageCI = {};
	imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCI.pNext = nullptr;
	imageCI.flags = 0;
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = format;
	imageCI.extent = extent;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = tiling;
	imageCI.usage = usage;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	{
		auto res = vkCreateImage(device, &imageCI, nullptr, &m_image);
		log_cond_err(res == VK_SUCCESS, "failed to create image");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, m_image, &memoryRequirements);

	VkMemoryAllocateInfo memoryAI = {};
	memoryAI.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAI.allocationSize = memoryRequirements.size;
	memoryAI.memoryTypeIndex = find_memory_type(physicalDevice, memoryRequirements.memoryTypeBits, memoryProperties);

	{
		auto res = vkAllocateMemory(device, &memoryAI, nullptr, &m_memory);
		log_cond_err(res == VK_SUCCESS, "failed to allocate image memory");
	}

	vkBindImageMemory(device, m_image, m_memory, 0);

	VkImageViewCreateInfo imageViewCI = {};
	imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCI.pNext = nullptr;
	imageViewCI.flags = 0;
	imageViewCI.image = m_image;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.aspectMask = aspect;

	{
		auto res = vkCreateImageView(device, &imageViewCI, nullptr, &m_imageView);
		log_cond_err(res == VK_SUCCESS, "failed to create image view");
	}
}
void VImage::destroy()
{
	vkDestroyImageView(m_device, m_imageView, nullptr);
	vkFreeMemory(m_device, m_memory, nullptr);
	vkDestroyImage(m_device, m_image, nullptr);
}