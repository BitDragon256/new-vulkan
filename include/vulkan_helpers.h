#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

// physical m_device
bool is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);
uint32_t rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface);

const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
bool check_device_extension_support(VkPhysicalDevice device);

//queues
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentationFamily;
	bool is_complete();
};
QueueFamilyIndices find_queue_families(VkPhysicalDevice device, VkSurfaceKHR surface);

// swapchain
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);

VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

std::vector<char> read_file(const std::string& filename);
VkShaderModule create_shader_module(const std::string& file, VkDevice device);

uint32_t find_memory_type(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkCommandBuffer begin_single_time_cmd_buffer(VkCommandPool cmdPool, VkDevice device);
void end_single_time_cmd_buffer(VkCommandBuffer commandBuffer, VkCommandPool cmdPool, VkDevice device, VkQueue queue);

void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
VkFormat find_depth_format(VkPhysicalDevice physicalDevice);

/*
* -------------------------------------------------
*			       OBJECT CREATION
* -------------------------------------------------
 */
VkSemaphore vk_create_semaphore(VkDevice device);
void vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore);