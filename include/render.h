#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <string>

#ifdef _MSC_BUILD
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include "buffer.h"
#include "ecs.h"
#include "nve_types.h"
#include "model-handler.h"
#include "gizmos.h"
#include "image.h"
#include "thread_pool.h"
#include "profiler.h"
#include "component-editor.h"

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

	Vector3 clearColor;

	bool enableValidationLayers;
	std::vector<const char*> enabledInstanceLayers;

	bool autoECSUpdate;

	RenderConfig() :
		width{ 600 }, height{ 400 }, title{ "DEFAULT TITLE" },
		dataMode{ TestTri }, cameraEnabled{ false },
		clearColor{ 0, 0, 0 },
		enableValidationLayers{ true },
		enabledInstanceLayers{ },
		autoECSUpdate{ true }
	{}
};

class Renderer;

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
	bool m_orthographic;

private:
	Renderer* renderer;
};

class Renderer
{
public:
	Renderer();

	NVE_RESULT render();
	NVE_RESULT init(RenderConfig config);
	NVE_RESULT set_active_camera(Camera* camera);

	ECSManager m_ecs;

	// input
	int get_key(int key);
	int get_mouse_button(int btn);
	Vector2 get_mouse_pos();
	Vector2 mouse_to_screen(Vector2 mouse);

	// gui
	void gui_begin();
	void draw_engine_gui(std::function<void(void)> guiDraw);

	void clean_up();

	void set_light_pos(Vector3 pos);

	// gizmos
	void gizmos_draw_line(Vector3 start, Vector3 end, Color color, float width = 0.1f);
      void gizmos_draw_ray(Vector3 start, Vector3 direction, Color color, float width = 0.1f);

	// ecs

	// create an entity with a transform attached to it
	EntityId create_empty_game_object();
	// create an entity with a transform and a dynamic model with the given default model attached to it
	EntityId create_default_model(DefaultModel::DefaultModel model);

private:
	void first_frame();
	bool m_firstFrame;

	float m_deltaTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime;

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
	bool m_acquireImageTimeout;

	std::vector<VImage> m_depthImages;
	void create_depth_images();

	VkQueue m_graphicsQueue;
	VkQueue m_presentationQueue;
	VkQueue m_transferQueue;
	VkQueue m_computeQueue;

	QueueFamilyIndices m_queueFamilyIndices;

	VkRenderPass m_renderPass;

	uint32_t m_frame;
	std::vector<VkFramebuffer> m_swapchainFramebuffers;

	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence> m_inFlightFences;

	VkDebugUtilsMessengerEXT m_debugMessenger;

	// model handling
	StaticGeometryHandler m_staticGeometryHandler;
	DynamicGeometryHandler m_dynamicGeometryHandler;
	GizmosHandler m_gizmosHandler;

	std::vector<GeometryHandler*> all_geometry_handlers();

	void create_geometry_pipelines();
	void initialize_geometry_handlers();
	void set_geometry_handler_subpasses();
	void update_geometry_handler_framebuffers();

// TODO staged buffer copy synchronization
	void wait_for_geometry_handler_buffer_cpies();

	uint32_t m_lastGeometryHandlerSubpassCount;
	uint32_t geometry_handler_subpass_count();

	void geometry_handler_cleanup();

	VkDescriptorPool m_descriptorPool;
	void create_descriptor_pool();

	// std::vector<VkDescriptorSetLayoutBinding> m_descriptors;
	// VkDescriptorSet m_descriptorSet;
	// VkDescriptorSetLayout m_descriptorSetLayout;
	
	// void add_descriptors();
	// void update_model_info_descriptor_set();

	// camera stuff
	Camera* m_activeCamera;
	Camera m_defaultCamera;
	void init_default_camera();
	CameraPushConstant m_cameraPushConstant;

	// imgui vulkan objects
	VkDescriptorPool m_imgui_descriptorPool;
	VkRenderPass m_imgui_renderPass;
	std::vector<VkFramebuffer> m_imgui_framebuffers;
	VkCommandPool m_imgui_commandPool;
	std::vector<VkCommandBuffer> m_imgui_commandBuffers;

	const VkClearValue m_imgui_clearColor = { {{ 0, 0, 0, 1.00f }} };

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
	void recreate_render_pass();
	NVE_RESULT create_framebuffers();
	NVE_RESULT create_commandpool();
	NVE_RESULT create_commandbuffers();
	NVE_RESULT create_sync_objects();
	
	// vulkan destruction
	void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	// rendering
	void record_main_command_buffer(uint32_t imageIndex);
	NVE_RESULT submit_command_buffers(std::vector<VkCommandBuffer> commandBuffers, std::vector<VkSemaphore> waitSems, std::vector<VkPipelineStageFlags> waitStages, std::vector<VkSemaphore> signalSems);
	void present_swapchain_image(VkSwapchainKHR swapchain, uint32_t imageIndex, std::vector<VkSemaphore> signalSems);
	NVE_RESULT draw_frame();

	GUIManager m_guiManager;

	// multithreading
	ThreadPool m_threadPool;
	const int m_threadCount = 2;
	void genCmdBuf(GeometryHandler* geometryHandler);

	// profiling
	Profiler m_profiler;
	float m_avgRenderTime;
};

void imgui_error_handle(VkResult err);