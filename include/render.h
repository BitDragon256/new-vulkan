#pragma once

#include <array>
#include <functional>
#include <string>

#ifdef _MSC_BUILD
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "buffer.h"
#include "models.h"
#include "nve_types.h"

#define NVE_MODEL_INFO_BUFFER_BINDING 0
#define NVE_MAX_MODEL_INFO_COUNT 1

struct RenderConfig
{
	int width;
	int height;
	std::string title;

	enum DataMode { TestTri = 0, VertexOnly = 1, Indexed = 2 };
	DataMode dataMode;

	bool cameraEnabled;
	bool useModelHandler;

	Vector3 clearColor;

	bool enableValidationLayers;
	std::vector<const char*> enabledInstanceLayers;

	RenderConfig() :
		width{ 600 }, height{ 400 }, title{ "DEFAULT TITLE" },
		dataMode{ Indexed }, cameraEnabled{ true }, useModelHandler{ true },
		clearColor{ 0, 0, 0 },
		enableValidationLayers{ true },
		enabledInstanceLayers {}
	{}
};

class Camera;

class Renderer
{
public:
	Renderer();
	NVE_RESULT render();
	NVE_RESULT init(RenderConfig config);
	NVE_RESULT bind_model_handler(ModelHandler* pHandler);
	NVE_RESULT set_vertices(const std::vector<Vertex>& vertices);
	NVE_RESULT set_indices(const std::vector<Index>& indices);
	NVE_RESULT set_active_camera(Camera* camera);
	void reload_models();

	// input
	int get_key(int key);

	// gui
	void gui_begin();

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

	VkPipelineLayout m_mainPipelineLayout;
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

	VkDebugUtilsMessengerEXT m_debugMessenger;

	// model handling
	ModelHandler* m_pModelHandler;

	VkDescriptorPool m_descriptorPool;
	std::vector<VkDescriptorSetLayoutBinding> m_descriptors;
	VkDescriptorSet m_descriptorSet;
	VkDescriptorSetLayout m_descriptorSetLayout;

	void add_descriptors();
	void update_model_info_descriptor_set();

	// camera stuff
	Camera* m_activeCamera;

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
	
	bool m_imguiDraw;
	void imgui_draw(uint32_t imageIndex);

	// GLFW objects
	GLFWwindow* m_window;
	
	// vulkan object creation
	NVE_RESULT create_instance();
	NVE_RESULT create_debug_messenger();
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
	
	// vulkan destruction
	void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	// rendering
	NVE_RESULT record_main_command_buffer(uint32_t imageIndex);
	NVE_RESULT submit_command_buffers(std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSems, std::vector<VkSemaphore> signalSems);
	void present_swapchain_image(VkSwapchainKHR swapchain, uint32_t imageIndex, std::vector<VkSemaphore> signalSems);
	NVE_RESULT draw_frame();
};

void imgui_error_handle(VkResult err);

class Texture
{
public:
	VkImage m_tex;
};

class Camera
{
public:
	Camera();
	glm::mat4 projection_matrix();
	glm::mat4 view_matrix();

	Vector3 m_position;
	Vector3 m_rotation;
	float m_fov;
	Vector2 m_extent;
	float m_nearPlane;
	float m_farPlane;

private:
	Renderer* renderer;
};

struct CameraPushConstant
{
	glm::mat4 view;
	glm::mat4 proj;
};