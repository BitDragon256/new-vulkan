#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "math-core.h"

#include "buffer.h"
#include "ecs.h"
#include "material.h"

#define GEOMETRY_HANDLER_MAX_MATERIALS 128

#define GEOMETRY_HANDLER_MATERIAL_BINDING 0
#define GEOMETRY_HANDLER_TEXTURE_BINDING 1
#define GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING 2

#define DYNAMIC_MODEL_HANDLER_TRANSFORM_BUFFER_BINDING 3

struct Transform
{
	alignas(16) Vector3 position;
	alignas(16) Vector3 scale;
	alignas(16) Quaternion rotation;

	Transform();
	Transform(Vector3 position, Vector3 scale, Quaternion rotation);
};

struct StaticMesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Material material;
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
};

struct MeshGroup // group with individual shaders
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Buffer<Vertex> vertexBuffer;
	Buffer<Index> indexBuffer;

	GraphicsShader shader;
	
	bool reloadMeshBuffers;

	VkPipeline pipeline;
	std::vector<VkCommandBuffer> commandBuffers;
};

struct GeometryHandlerVulkanObjects
{
	VkDevice device;
	VkCommandPool commandPool;
	VkRenderPass renderPass;
	uint32_t firstSubpass;

	std::vector<VkFramebuffer> framebuffers;
	VkExtent2D swapchainExtent;

	VkPhysicalDevice physicalDevice;
	VkQueue transferQueue;
	
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

	void initialize(GeometryHandlerVulkanObjects vulkanObjects);
	void update_framebuffers(std::vector<VkFramebuffer> framebuffers, VkExtent2D swapchainExtent);

	uint32_t subpass_count();

	void cleanup();

protected:

	void add_model(Model& model, Transform transform);
	virtual void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup) = 0;
	
	void update();

	GeometryHandlerVulkanObjects m_vulkanObjects;
	VkPipelineLayout m_pipelineLayout;
	VkDescriptorSet m_descriptorSet;

	BufferConfig default_buffer_config();

	virtual std::vector<VkDescriptorSetLayoutBinding> other_descriptors() = 0;

private:

	MeshGroup* find_group(GraphicsShader*& shader, size_t& index);
	MeshGroup* push_mesh_group(GraphicsShader*& shader);

	void create_group_command_buffers(MeshGroup& meshGroup);

	VkGraphicsPipelineCreateInfo create_pipeline_create_info(uint32_t subpass, size_t pipelineIndex);
	void create_pipeline_layout();

	std::vector<MeshGroup> m_meshGroups;
	std::vector<MeshDataInfo> m_meshes;
	std::vector<Material> m_materials;
	Buffer<MaterialSSBO> m_materialBuffer;
	VkWriteDescriptorSet material_buffer_descriptor_set_write();
	VkDescriptorBufferInfo m_materialBufferDescriptorInfo;

	std::vector<PipelineCreationData> m_pipelineCreationData;

	bool reloadMeshBuffers;

	void reload_meshes();
	void reload_mesh_group(MeshGroup& meshGroup);

	VkDescriptorSetLayout m_descriptorSetLayout;

	void create_descriptor_set();
	void update_descriptor_set();

	TexturePool m_texturePool;
};

struct StaticModel : Model
{
	
};

class StaticGeometryHandler : public GeometryHandler, System<StaticModel, Transform>
{
public:

	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

protected:

	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup) override;
	std::vector<VkDescriptorSetLayoutBinding> other_descriptors() override;

private:

	void add_model(StaticModel& model, Transform transform);
};

struct DynamicModel : Model
{

};

class DynamicGeometryHandler : public GeometryHandler, System<DynamicModel, Transform>
{
public:

	void start(ECSManager& ecs) override;
	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

protected:

	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup) override;
	std::vector<VkDescriptorSetLayoutBinding> other_descriptors() override;

private:

	void add_model(DynamicModel& model, Transform transform);

	Buffer<Transform> m_transformBuffer;
};