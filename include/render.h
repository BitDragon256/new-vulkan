#pragma once

#include <array>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "buffer.h"
#include "models.h"
#include "nve_types.h"

typedef struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
} Vertex;
typedef uint32_t Index;

struct RenderConfig
{
	int width;
	int height;
	std::string title;

	bool vertexOnlyMode;
	bool vertexIndexMode;
};

class Renderer
{
public:
	NVE_RESULT render();
	NVE_RESULT init(RenderConfig config);
	NVE_RESULT bind_model_handler(ModelHandler* handler);
	NVE_RESULT set_vertices(const std::vector<Vertex>& vertices);
	NVE_RESULT set_indices(const std::vector<Index>& indices);

	void clean_up();

private:
	RenderConfig m_config;

	// vulkan objects
	VkInstance m_instance;
	VkPhysicalDevice m_physicalDevice;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapchain;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainImageViews;
	VkFormat m_swapchainImageFormat;
	VkExtent2D m_swapchainExtent;

	VkQueue m_graphicsQueue;
	VkQueue m_presentationQueue;

	VkRenderPass m_renderPass;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;

	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkCommandPool m_commandPool;
	VkCommandBuffer m_commandBuffer;

	VkSemaphore m_imageAvailableSemaphore;
	VkSemaphore m_renderFinishedSemaphore;
	VkFence m_inFlightFence;

	std::vector<Vertex> m_vertices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	bool m_vertexBufferCreated;

	std::vector<Index> m_indices;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
	bool m_indexBufferCreated;

	// GLFW objects
	GLFWwindow* m_window;
	
	// vulkan object creation
	NVE_RESULT create_instance();
	NVE_RESULT get_physical_device();
	NVE_RESULT create_device();
	NVE_RESULT create_window(int width, int height, std::string title);
	NVE_RESULT get_surface();
	NVE_RESULT create_swapchain();
	NVE_RESULT create_swapchain_image_views();
	NVE_RESULT create_render_pass();
	NVE_RESULT create_graphics_pipeline();
	NVE_RESULT create_framebuffers();
	NVE_RESULT create_commandpool();
	NVE_RESULT create_commandbuffer();
	NVE_RESULT create_sync_objects();
	NVE_RESULT create_vertex_buffer(uint32_t size);

	// rendering
	NVE_RESULT record_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	NVE_RESULT draw_frame();
};
