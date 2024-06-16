#pragma once

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <GLFW/glfw3.h>

#include "reference.h"
#include "vulkan/vulkan_handles_fwd.h"

#define VK_CHECK_ERROR(res) { vk_check_error(res, __FILE__, __LINE__); }
inline void vk_check_error(VkResult result, const char* file, int line)
{
      if (result != VK_SUCCESS)
            fprintf(stderr, "vulkan error in file %s, line %d: %s\n", file, line, string_VkResult(result));
}


// physical device
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
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> computeFamily;
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
* ---------------------------------
*         OBJECT CREATION
* ---------------------------------
 */
VkSemaphore vk_create_semaphore(VkDevice device);
void vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore);

VkBufferCreateInfo make_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode);
VkBuffer make_buffer(REF(vk::Device) device, VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode);
void destroy_buffer(REF(vk::Device) device, VkBuffer buffer);
VkDeviceMemory allocate_buffer_memory(REF(vk::Device) device, VkBuffer buffer, VkMemoryPropertyFlags propertyFlags);
VkDeviceAddress get_buffer_device_address(REF(vk::Device) device, VkBuffer buffer);

void free_device_memory(REF(vk::Device) device, VkDeviceMemory memory);

void* get_pnext(void* structure);
bool pnext_is_null(void* structure);
VkStructureType get_structure_type(void* structure);
VkBaseOutStructure* to_base_structure(void* structure);
template<typename T>
inline void get_physical_device_extension_properties(REF(vk::PhysicalDevice) physicalDevice, VkStructureType type, T& properties)
{
      VkPhysicalDeviceProperties2 allProperties;
      vkGetPhysicalDeviceProperties2(*physicalDevice, &allProperties);

      void* traversalStructure = (void*) & allProperties;

      while (!pnext_is_null(traversalStructure))
      {
            traversalStructure = get_pnext(traversalStructure);
            if (get_structure_type(traversalStructure) == type)
            {
                  properties = *reinterpret_cast<T*>(traversalStructure);
                  return;
            }
      }
}

uint32_t aligned_size(uint32_t value, uint32_t alignment);