#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "math-core.h"

#include "buffer.h"
#include "ecs.h"
#include "material.h"

struct Transform
{
	alignas(16) Vector3 position;
	alignas(16) Vector3 scale;
	alignas(16) Quaternion rotation;
};

struct StaticMesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Material material;
};
struct MeshDataInfo
{
	size_t vertexStart;
	size_t vertexCount;
	size_t indexStart;
	size_t indexCount;

	size_t meshGroup;
};

struct MeshGroup
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
	Material material;

	Buffer<Vertex> vertexBuffer;
	Buffer<Index> indexBuffer;
	bool reloadMeshBuffers;

	VkPipeline pipeline;
	bool rerecordBuffer;
	std::vector<VkCommandBuffer> commandBuffers;
};

struct StaticGeometryHandlerVulkanObjects
{
	VkDevice device;
	VkCommandPool commandPool;
	VkRenderPass renderPass;
	uint32_t firstSubpass;

	std::vector<VkFramebuffer> framebuffers;
	VkExtent2D swapchainExtent;

	VkPhysicalDevice physicalDevice;
	VkQueue transferQueue;
};

struct PipelineCreationData
{
	VkPipelineShaderStageCreateInfo			stages[2];

	VkPipelineVertexInputStateCreateInfo	vertexInputState;
	std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;
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

class StaticGeometryHandler : System<StaticMesh, Transform>
{
public:
	void create_command_buffers();
	void record_command_buffers();
	std::vector<VkCommandBuffer> get_command_buffers(uint32_t frame);

	void create_pipelines(std::vector<VkPipeline>& pipelines, std::vector<VkGraphicsPipelineCreateInfo>& createInfos);

	void initialize(StaticGeometryHandlerVulkanObjects vulkanObjects);
	void update_framebuffers(std::vector<VkFramebuffer> framebuffers, VkExtent2D swapchainExtent);

	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

	uint32_t subpass_count();

private:
	void add_mesh(StaticMesh& mesh, Transform transform);
	MeshGroup* find_group(const Material& material, size_t& index);

	void create_command_buffer(VkCommandBuffer* pCommandBuffer);
	void record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup);

	VkGraphicsPipelineCreateInfo create_pipeline_create_info(uint32_t subpass, size_t pipelineIndex);
	void create_pipeline_layout();

	std::vector<MeshGroup> m_meshGroups;
	std::vector<MeshDataInfo> m_meshes;

	std::vector<PipelineCreationData> m_pipelineCreationData;
	VkPipelineLayout m_pipelineLayout;

	bool reloadMeshBuffers;

	void reload_meshes();
	void reload_mesh_group(MeshGroup& meshGroup);

	BufferConfig default_buffer_config();
	StaticGeometryHandlerVulkanObjects m_vulkanObjects;
};