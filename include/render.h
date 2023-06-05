#pragma once

#include <array>
#include <functional>
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
#define NVE_INDEX_TYPE VK_INDEX_TYPE_UINT32

struct RenderConfig
{
	int width;
	int height;
	std::string title;

	enum DataMode { TestTri = 0, VertexOnly = 1, Indexed = 2 };
	DataMode dataMode;

	std::vector<const char*> enabledValidationLayers;

	// gui
	std::function<void(void)> guiDraw;
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

	QueueFamilyIndices m_queueFamilyIndices;

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
	Buffer<Vertex> m_vertexBuffer;

	std::vector<Index> m_indices;
	Buffer<Index> m_indexBuffer;

	// imgui vulkan objects
	VkDescriptorPool m_imgui_descriptorPool;
	VkRenderPass m_imgui_renderPass;
	std::vector<VkFramebuffer> m_imgui_framebuffers;
	VkCommandPool m_imgui_commandPool;
	std::vector<VkCommandBuffer> m_imgui_commandBuffers;

	const VkClearValue m_imgui_clearColor = { {{ 0.45f, 0.55f, 0.60f, 1.00f }} };

	// imgui methods
	NVE_RESULT imgui_init();
	NVE_RESULT imgui_create_render_pass();
	NVE_RESULT imgui_create_framebuffers();
	NVE_RESULT imgui_create_descriptor_pool();
	NVE_RESULT imgui_upload_fonts();
	NVE_RESULT imgui_create_command_pool();
	NVE_RESULT imgui_create_command_buffers();

	void imgui_cleanup();
	
	void imgui_demo_draws(uint32_t imageIndex);

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
	NVE_RESULT init_vertex_buffer();
	NVE_RESULT init_index_buffer();

	// additional creation
	VkCommandBuffer begin_single_time_cmd_buffer(VkCommandPool cmdPool);
	void end_single_time_cmd_buffer(VkCommandBuffer commandBuffer, VkCommandPool cmdPool);

	// rendering
	NVE_RESULT record_main_command_buffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	NVE_RESULT submit_command_buffers(std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSems, std::vector<VkSemaphore> signalSems);
	void present_swapchain_image(VkSwapchainKHR swapchain, uint32_t imageIndex, std::vector<VkSemaphore> signalSems);
	NVE_RESULT draw_frame();
};

void imgui_error_handle(VkResult err);
