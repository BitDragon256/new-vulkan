#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "ecs.h"
#include "nve_types.h"
#include "material.h"

#include "buffer.h"

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

	Buffer<Vertex> m_vertexBuffer;
	Buffer<Index> m_indexBuffer;

	VkPipeline pipeline;
	bool rerecordBuffer;
	VkCommandBuffer commandBuffer;
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
	void create_command_buffers(VkDevice device, VkCommandPool commandPool);
	void record_command_buffers(VkRenderPass renderPass, uint32_t firstSubpass, VkFramebuffer framebuffer, VkExtent2D swapChainExtent);
	void create_pipelines(VkDevice device, VkRenderPass renderPass, uint32_t firstSubpass, std::vector<VkPipeline>& pipelines, std::vector<VkGraphicsPipelineCreateInfo>& createInfos);

	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

private:
	void add_mesh(StaticMesh& mesh, Transform transform);
	MeshGroup* find_group(const Material& material, size_t& index);

	void create_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* pCommandBuffer);
	void record_command_buffer(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkExtent2D swapChainExtent, const MeshGroup& meshGroup);

	VkGraphicsPipelineCreateInfo create_pipeline_create_info(VkDevice device, VkRenderPass renderPass, uint32_t subpass, size_t pipelineIndex);
	void create_pipeline_layout(VkDevice device);

	std::vector<MeshGroup> m_meshGroups;
	std::vector<MeshDataInfo> m_meshes;

	std::vector<PipelineCreationData> m_pipelineCreationData;
	VkPipelineLayout m_pipelineLayout;
};