#pragma once

#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "math-core.h"
#include "gui.h"
#include "logger.h"
#include "profiler.h"

#include "buffer.h"
#include "ecs.h"
#include "material.h"
#include "math-core.h"

#define GEOMETRY_HANDLER_MAX_MATERIALS 128

#define GEOMETRY_HANDLER_MATERIAL_BINDING 0
#define GEOMETRY_HANDLER_TEXTURE_BINDING 1
#define GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING 2

#define DYNAMIC_MODEL_HANDLER_TRANSFORM_BUFFER_BINDING 3

#define GEOMETRY_HANDLER_INDEPENDENT_MATERIALS true

struct StaticMesh : Mesh
{
	std::shared_ptr<Material> material;
	size_t id;

	StaticMesh();
};
struct Model
{
	std::vector<StaticMesh> m_children;

	void load_mesh(std::string file);
};
struct MeshDataInfo
{
	size_t vertexStart;
	size_t vertexCount;
	size_t indexStart;
	size_t indexCount;

	size_t meshGroup;
	size_t meshId;
};
bool operator== (const MeshDataInfo& a, const MeshDataInfo& b);

struct MeshGroup // group with individual shaders
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Buffer<Vertex> vertexBuffer;
	Buffer<Index> indexBuffer;

	std::vector<MeshDataInfo> meshes;

	GraphicsShaderRef shader;
	
	bool reloadMeshBuffers;

	VkPipeline pipeline;
	std::vector<VkCommandBuffer> commandBuffers;
	VkCommandPool commandPool;
};

struct GeometryHandlerVulkanObjects
{
	VkDevice device;
	VkCommandPool commandPool;
	VkRenderPass* renderPass; // renderPass can change
	uint32_t firstSubpass;

	std::vector<VkFramebuffer> framebuffers;
	VkExtent2D swapchainExtent;

	VkPhysicalDevice physicalDevice;
	VkQueue transferQueue;
	uint32_t queueFamilyIndex;
	
	VkDescriptorPool descriptorPool;

	CameraPushConstant* pCameraPushConstant;
};

struct PipelineCreationData
{
	VkPipelineShaderStageCreateInfo			stages[2];

	VkPipelineVertexInputStateCreateInfo	vertexInputState;
	std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> vertexAttributeDescriptions;
	VkVertexInputBindingDescription			vertexBindingDescription;

	VkPipelineInputAssemblyStateCreateInfo	inputAssemblyState;
	VkPipelineTessellationStateCreateInfo	tessellationState;
	VkPipelineViewportStateCreateInfo		viewportState;
	VkPipelineRasterizationStateCreateInfo	rasterizationState;
	VkPipelineMultisampleStateCreateInfo	multisampleState;
	VkPipelineDepthStencilStateCreateInfo	depthStencilState;

	VkPipelineColorBlendStateCreateInfo		colorBlendState;
	VkPipelineColorBlendAttachmentState		colorBlendAttachment;

	VkPipelineDynamicStateCreateInfo		dynamicState;
	std::vector<VkDynamicState>				dynamicStates;
};

class GeometryHandler
{
public:
	// void create_command_buffers();
	void record_command_buffers(uint32_t frame);
	std::vector<VkCommandBuffer> get_command_buffers(uint32_t frame);

	void create_pipeline_create_infos(std::vector<VkGraphicsPipelineCreateInfo>& createInfos);
	void set_pipelines(std::vector<VkPipeline>& pipelines);

	GeometryHandler();
	virtual void initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* guiManager);
	void update_framebuffers(std::vector<VkFramebuffer> framebuffers, VkExtent2D swapchainExtent);
	void set_first_subpass(uint32_t subpass);

	uint32_t subpass_count();
	virtual std::vector<VkSemaphore> buffer_cpy_semaphores();
	virtual std::vector<VkFence> buffer_cpy_fences();

	virtual void cleanup();

protected:

	void add_model(Model& model, bool forceNewMeshGroup = false);
	void remove_model(Model& model);
	void add_material(Model& model, Transform& transform, bool newMat);
	virtual void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex) = 0;
	
	void update();

	GeometryHandlerVulkanObjects m_vulkanObjects;
	VkPipelineLayout m_pipelineLayout;
	VkDescriptorSet m_descriptorSet;

	BufferConfig default_buffer_config();

	virtual std::vector<VkDescriptorSetLayoutBinding> other_descriptors() = 0;

	GUIManager* m_guiManager;
	uint32_t m_subpassCount;

	Profiler m_profiler;

private:

	MeshGroup* find_group(GraphicsShader& shader, size_t& index);
	MeshGroup* push_mesh_group(GraphicsShader& shader);

	void create_group_command_buffers(MeshGroup& meshGroup);

	VkGraphicsPipelineCreateInfo create_pipeline_create_info(uint32_t subpass, size_t pipelineIndex);
	void create_pipeline_layout();
	void create_pipeline(size_t meshGroupIndex);

	bool m_rendererPipelinesCreated;

	std::vector<MeshGroup> m_meshGroups;
	std::vector<std::shared_ptr<Material>> m_materials;
	Buffer<MaterialSSBO> m_materialBuffer;
	VkWriteDescriptorSet material_buffer_descriptor_set_write();
	VkDescriptorBufferInfo m_materialBufferDescriptorInfo;

	std::vector<PipelineCreationData> m_pipelineCreationData;

	bool reloadMeshBuffers;

	void reload_meshes();
	void reload_mesh_group(MeshGroup& meshGroup);

	void reload_materials();

	VkDescriptorSetLayout m_descriptorSetLayout;

	void create_descriptor_set();
	void update_descriptor_set();

	TexturePool m_texturePool;

	//static std::set<GraphicsShader_T*> s_destroyedShaders;
};

struct StaticModel : Model
{
	
};

class StaticGeometryHandler : public GeometryHandler, System<StaticModel, Transform>
{
public:

	StaticGeometryHandler();
	void initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* guiManager);
	void awake(EntityId entity) override;
	void update(float dt) override;

protected:

	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex) override;
	std::vector<VkDescriptorSetLayoutBinding> other_descriptors() override;

private:

	StaticModel m_dummyModel;
	void load_dummy_model();
	void add_model(StaticModel& model, Transform& transform);
};

typedef uint64_t DynamicModelHashSum;

struct DynamicModel : Model
{
	DynamicModelHashSum hashSum;
};

DynamicModelHashSum hash_model(const DynamicModel& model);

struct DynamicModelInfo
{
	uint32_t startIndex;
	uint32_t instanceCount;
	DynamicModelHashSum hashSum;
};

class DynamicGeometryHandler : public GeometryHandler, System<DynamicModel, Transform>
{
public:

	DynamicGeometryHandler();

	void start() override;
	void awake(EntityId entity) override;
	void update(float dt) override;

	std::vector<VkSemaphore> buffer_cpy_semaphores() override;
	std::vector<VkFence> buffer_cpy_fences() override;

	void cleanup() override;

protected:

	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex) override;
	std::vector<VkDescriptorSetLayoutBinding> other_descriptors() override;

private:

	void add_model(DynamicModel& model, Transform& transform);

	Buffer<Transform> m_transformBuffer;

	std::vector<DynamicModelInfo> m_individualModels;
	uint32_t m_modelCount;
};

// ---------------------------------
// HELPER FUNCTIONS
// ---------------------------------

void bake_transform(StaticMesh& mesh, Transform transform);
VkCommandBufferBeginInfo create_command_buffer_begin_info(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkCommandBufferInheritanceInfo& inheritanceI);
void set_dynamic_state(VkCommandBuffer commandBuffer, VkExtent2D swapChainExtent, std::array<float, 4> viewportSize);

// ---------------------------------
// DEFAULT MODELS
// ---------------------------------
namespace DefaultModel
{
	enum DefaultModel
	{
		Cube, Sphere, Quad, Triangle
	};
}
inline std::unordered_map<DefaultModel::DefaultModel, std::string> s_defaultModelToPath = {
	{ DefaultModel::Cube, "/default_models/cube.obj" },
	{ DefaultModel::Sphere, "/default_models/sphere.obj" },
	{ DefaultModel::Quad, "/default_models/quad.obj" },
	{ DefaultModel::Triangle, "/default_models/triangle.obj" }
};
