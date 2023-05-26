#pragma once

#include <string>

#include <vulkan/vulkan.hpp>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "models.h"

typedef int NVE_RESULT;
#define NVE_SUCCESS 0
#define NVE_FAILURE -1

struct RenderConfig
{
	int width;
	int height;
	std::string title;
};

class Renderer
{
public:
	NVE_RESULT render();
	NVE_RESULT init(RenderConfig config);
	NVE_RESULT bind_model_handler(ModelHandler* handler);

private:
	// vulkan objects
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;

	VkQueue m_graphicsQueue;
	VkQueue m_presentationQueue;

	// GLFW objects
	GLFWwindow* m_window;
	
	// vulkan object creation
	NVE_RESULT create_instance();
	NVE_RESULT get_physical_device();
	NVE_RESULT create_device();
	NVE_RESULT create_window(int width, int height, std::string title);
	NVE_RESULT get_surface();
	NVE_RESULT create_swapchain(int height, int width);
	NVE_RESULT create_image_views();
	NVE_RESULT create_pipeline();

	void clean_up();

	// rendering
};
