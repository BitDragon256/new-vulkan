#pragma once

#include <array>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "nve_types.h"
#include "material.h"

using RenderPassRef = std::shared_ptr<VkRenderPass>;

class PipelineLayout
{
public:

      void create(std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants, VkDevice device);

      VkPipelineLayout m_layout;

private:
};

typedef std::shared_ptr<PipelineLayout> PipelineLayoutRef;

enum PipelineType { GraphicsPipeline_E = 0, RayTracingPipeline_E = 1, ComputePipeline_E = 2 };

class Pipeline
{
public:

	Pipeline(PipelineType type);
	void initialize(VkDevice device, PipelineLayoutRef layout);
	virtual void create_create_info() = 0;
	virtual void create() = 0;
	void destroy();

	const PipelineType m_type;

	VkDevice m_device;

      VkPipeline m_pipeline;
      PipelineLayoutRef m_layout;

};

typedef std::shared_ptr<Pipeline> PipelineRef;

typedef struct GraphicsPipelineCreationData_T
{
	std::vector<VkPipelineShaderStageCreateInfo>	stages;

	VkPipelineVertexInputStateCreateInfo	      vertexInputState;
	std::array<
		VkVertexInputAttributeDescription,
		VERTEX_ATTRIBUTE_COUNT
	>						      	vertexAttributeDescriptions;
	VkVertexInputBindingDescription	      	vertexBindingDescription;

	VkPipelineInputAssemblyStateCreateInfo		inputAssemblyState;
	VkPipelineTessellationStateCreateInfo		tessellationState;
	VkPipelineViewportStateCreateInfo			viewportState;
	VkPipelineRasterizationStateCreateInfo		rasterizationState;
	VkPipelineMultisampleStateCreateInfo		multisampleState;
	VkPipelineDepthStencilStateCreateInfo		depthStencilState;

	VkPipelineColorBlendStateCreateInfo			colorBlendState;
	VkPipelineColorBlendAttachmentState			colorBlendAttachment;

	VkPipelineDynamicStateCreateInfo			dynamicState;
	std::vector<VkDynamicState>				dynamicStates;
} GraphicsPipelineCreationData;

class GraphicsPipeline : public Pipeline
{
public:

	GraphicsPipeline();
	void create_create_info() override;
	void create() override;

	void initialize(RenderPassRef renderPass, uint32_t subpass);

	VkGraphicsPipelineCreateInfo m_createInfo;

private:

	GraphicsPipelineCreationData m_creationData;
	void set_default_create_info();
	void set_shader_stages();

	uint32_t m_subpass;
	RenderPassRef m_renderPass;
	GraphicsShader m_shader;

};

class PipelineBatchCreator
{
public:

	PipelineBatchCreator();

	void schedule_creation(PipelineRef pipeline);
	void create_all();

private:

	void create_cache();
	void create_graphics_pipelines();
	void create_raytracing_pipelines();
	void create_compute_pipelines();

	std::vector<PipelineRef> m_pipelines;

	VkPipelineCache m_cache;
	VkDevice m_device;
	bool m_cacheCreated;

};
