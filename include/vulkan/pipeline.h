#pragma once

#include <array>
#include <memory>
#include <vector>

#include <vulkan/vulkan.h>

#include "reference.h"

#include "nve_types.h"
#include "material.h"
#include "vulkan/vulkan_handles.h"

namespace vk
{

	class PipelineLayout
	{
	public:

		void create(std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::vector<VkPushConstantRange> pushConstants, VkDevice device);
		void destroy();

		operator VkPipelineLayout() const;

		VkPipelineLayout m_layout;

	private:

		VkDevice m_device;

	};

	typedef Reference<PipelineLayout> PipelineLayoutRef;

	enum PipelineType { GraphicsPipeline_E = 0, RayTracingPipeline_E = 1, ComputePipeline_E = 2 };

	class Pipeline
	{
	public:

		Pipeline(PipelineType type);
		virtual void create_create_info() = 0;
		virtual void create() = 0;
		void destroy();
		void set_created();

		const PipelineType m_type;

		VkDevice m_device;

		VkPipeline m_pipeline;
		PipelineLayoutRef m_layout;

		operator VkPipeline() const;

	protected:

		void initialize(VkDevice device, PipelineLayoutRef layout);

		bool m_created;
	};

	typedef Reference<Pipeline> PipelineRef;

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

		void initialize(
			VkDevice device,
			PipelineLayoutRef layout,
			REF(RenderPass) renderPass,
			uint32_t subpass,
			GraphicsShaderRef shader
		);

		VkGraphicsPipelineCreateInfo m_createInfo;

	private:

		GraphicsPipelineCreationData m_creationData;
		void set_default_create_info();
		void set_shader_stages();

		uint32_t m_subpass;
		REF(RenderPass) m_renderPass;
		GraphicsShaderRef m_shader;

	};

	typedef struct RayTracingPipelineCreationData_T
	{
		VkPipelineDynamicStateCreateInfo dynamicState;
		std::vector<VkDynamicState> dynamicStates;

		std::vector<VkPipelineShaderStageCreateInfo> stageCIs;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groupCIs;
	} RayTracingPipelineCreationData;

	class RayTracingPipeline : public Pipeline
	{
	public:

		RayTracingPipeline();
		void create_create_info() override;
		void create() override;

		void initialize(
			VkDevice device,
			PipelineLayoutRef layout,
			RayTracingShader shader
		);

		VkRayTracingPipelineCreateInfoKHR m_createInfo;

	private:

		RayTracingPipelineCreationData m_creationData;

		void set_default_create_info();
		void set_shader_stages();

		RayTracingShader m_shader;

	};

	class PipelineBatchCreator
	{
	public:

		PipelineBatchCreator();

		void schedule_creation(PipelineRef pipeline);
		void create_all();
		void destroy();

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

} // namespace vk
