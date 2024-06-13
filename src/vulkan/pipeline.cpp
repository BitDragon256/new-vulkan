#include "vulkan/pipeline.h"

#include <functional>
#include <unordered_map>

namespace vk
{

	// -------------------------------------------
	// PIPELINE LAYOUT
	// -------------------------------------------

	void PipelineLayout::create(
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
		std::vector<VkPushConstantRange> pushConstants,
		VkDevice device
	)
	{
		m_device = device;

		VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.pNext = nullptr;
		pipelineLayoutCI.flags = 0;

		pipelineLayoutCI.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());

		pipelineLayoutCI.pPushConstantRanges = pushConstants.data();
		pipelineLayoutCI.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());

		auto res = vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &m_layout);
	}
	void PipelineLayout::destroy()
	{
		vkDestroyPipelineLayout(m_device, m_layout, nullptr);
	}
	PipelineLayout::operator VkPipelineLayout() const
	{
		return m_layout;
	}

	// --------------------------------------
	// PIPELINE
	// --------------------------------------

	Pipeline::Pipeline(PipelineType type) :
		m_type{ type }, m_created{ false },
		m_pipeline{ VK_NULL_HANDLE }, m_layout{ VK_NULL_HANDLE }
	{

	}
	void Pipeline::initialize(VkDevice device, PipelineLayoutRef layout)
	{
		m_device = device;
		m_layout = layout;
	}
	void Pipeline::destroy()
	{
		vkDestroyPipeline(
			m_device,
			m_pipeline,
			nullptr
		);
	}
	void Pipeline::set_created()
	{
		m_created = true;
	}
	Pipeline::operator VkPipeline() const
	{
		return m_pipeline;
	}

	// --------------------------------------
	// GRAPHICS PIPELINE
	// --------------------------------------

	GraphicsPipeline::GraphicsPipeline() :
		Pipeline::Pipeline(GraphicsPipeline_E), m_renderPass{}, m_createInfo{}, m_subpass{ 0 }, m_shader{}
	{

	}
	void GraphicsPipeline::initialize(VkDevice device, PipelineLayoutRef layout, REF(RenderPass) renderPass, uint32_t subpass, GraphicsShaderRef shader)
	{
		Pipeline::initialize(device, layout);

		m_renderPass = renderPass;
		m_subpass = subpass;
		m_shader = shader;
	}
	void GraphicsPipeline::create_create_info()
	{
		if (m_created)
			destroy();

		set_default_create_info();
		set_shader_stages();

		m_createInfo.layout = m_layout->m_layout;
		m_createInfo.renderPass = *m_renderPass;
		m_createInfo.subpass = m_subpass;
	}
	void GraphicsPipeline::create()
	{
		vkCreateGraphicsPipelines(
			m_device, VK_NULL_HANDLE,
			1,
			&m_createInfo,
			nullptr,
			&m_pipeline
		);

		m_created = true;
	}

	void GraphicsPipeline::set_default_create_info()
	{
		m_createInfo = {};
		m_createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = 0;

		// ---------------------------------------

		m_creationData.vertexInputState = {};
		m_creationData.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		m_creationData.vertexInputState.pNext = nullptr;
		m_creationData.vertexInputState.flags = 0;

		m_creationData.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
		m_creationData.vertexInputState.vertexAttributeDescriptionCount = 0;
		m_creationData.vertexInputState.vertexAttributeDescriptionCount = m_creationData.vertexAttributeDescriptions.size();
		m_creationData.vertexInputState.pVertexAttributeDescriptions = m_creationData.vertexAttributeDescriptions.data();

		m_creationData.vertexBindingDescription = Vertex::getBindingDescription();
		m_creationData.vertexInputState.vertexBindingDescriptionCount = 0;
		m_creationData.vertexInputState.vertexBindingDescriptionCount = 1;
		m_creationData.vertexInputState.pVertexBindingDescriptions = &m_creationData.vertexBindingDescription;

		m_createInfo.pVertexInputState = &m_creationData.vertexInputState;

		// -------------------------------------------

		m_creationData.inputAssemblyState = {};
		m_creationData.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		m_creationData.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		m_creationData.inputAssemblyState.primitiveRestartEnable = VK_FALSE;

		m_createInfo.pInputAssemblyState = &m_creationData.inputAssemblyState;

		// -------------------------------------------

		m_creationData.viewportState = {};
		m_creationData.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		m_creationData.viewportState.viewportCount = 1;
		m_creationData.viewportState.scissorCount = 1;

		m_createInfo.pViewportState = &m_creationData.viewportState;

		// -------------------------------------------

		m_creationData.rasterizationState = {};
		m_creationData.rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		m_creationData.rasterizationState.depthClampEnable = VK_FALSE;
		m_creationData.rasterizationState.rasterizerDiscardEnable = VK_FALSE;
		m_creationData.rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		m_creationData.rasterizationState.lineWidth = 1.0f;
		m_creationData.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
		m_creationData.rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
		m_creationData.rasterizationState.depthBiasEnable = VK_FALSE;

		m_createInfo.pRasterizationState = &m_creationData.rasterizationState;

		// -------------------------------------------

		m_creationData.multisampleState = {};
		m_creationData.multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		m_creationData.multisampleState.sampleShadingEnable = VK_FALSE;
		m_creationData.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		m_createInfo.pMultisampleState = &m_creationData.multisampleState;

		// -------------------------------------------

		m_creationData.depthStencilState = {};
		m_creationData.depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		m_creationData.depthStencilState.pNext = nullptr;
		m_creationData.depthStencilState.flags = 0;
		m_creationData.depthStencilState.depthTestEnable = VK_TRUE;
		m_creationData.depthStencilState.depthWriteEnable = VK_TRUE;
		m_creationData.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		m_creationData.depthStencilState.depthBoundsTestEnable = VK_FALSE;
		m_creationData.depthStencilState.stencilTestEnable = VK_FALSE;

		m_createInfo.pDepthStencilState = &m_creationData.depthStencilState;

		// -------------------------------------------

		m_creationData.colorBlendAttachment = {};
		m_creationData.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		m_creationData.colorBlendAttachment.blendEnable = VK_TRUE;
		m_creationData.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		m_creationData.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		m_creationData.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		m_creationData.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		m_creationData.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		m_creationData.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		m_creationData.colorBlendState = {};
		m_creationData.colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		m_creationData.colorBlendState.logicOpEnable = VK_FALSE;
		m_creationData.colorBlendState.logicOp = VK_LOGIC_OP_COPY;
		m_creationData.colorBlendState.attachmentCount = 1;
		m_creationData.colorBlendState.pAttachments = &m_creationData.colorBlendAttachment;
		m_creationData.colorBlendState.blendConstants[0] = 0.0f;
		m_creationData.colorBlendState.blendConstants[1] = 0.0f;
		m_creationData.colorBlendState.blendConstants[2] = 0.0f;
		m_creationData.colorBlendState.blendConstants[3] = 0.0f;

		m_createInfo.pColorBlendState = &m_creationData.colorBlendState;

		// -------------------------------------------

		m_creationData.dynamicStates = {
		   VK_DYNAMIC_STATE_VIEWPORT,
		   VK_DYNAMIC_STATE_SCISSOR
		};
		m_creationData.dynamicState = {};
		m_creationData.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_creationData.dynamicState.dynamicStateCount = static_cast<uint32_t>(m_creationData.dynamicStates.size());
		m_creationData.dynamicState.pDynamicStates = m_creationData.dynamicStates.data();

		m_createInfo.pDynamicState = &m_creationData.dynamicState;

		// -------------------------------------------

		m_createInfo.basePipelineHandle = VK_NULL_HANDLE;
		m_createInfo.basePipelineIndex = -1;

		// -------------------------------------------
	}
	void GraphicsPipeline::set_shader_stages()
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = m_shader.lock()->vertex.m_module;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = m_shader.lock()->fragment.m_module;
		fragShaderStageInfo.pName = "main";

		m_creationData.stages.clear();

		m_creationData.stages.push_back(vertShaderStageInfo);
		m_creationData.stages.push_back(fragShaderStageInfo);

		m_createInfo.stageCount = static_cast<uint32_t>(m_creationData.stages.size());
		m_createInfo.pStages = m_creationData.stages.data();
	}

	// --------------------------------------
	// RAY TRACING PIPELINE
	// --------------------------------------

	RayTracingPipeline::RayTracingPipeline() :
		Pipeline::Pipeline{ RayTracingPipeline_E }
	{}

	void RayTracingPipeline::initialize(
		VkDevice device,
		PipelineLayoutRef layout,
		RayTracingShader shader
	)
	{
		Pipeline::initialize(device, layout);

		m_shader = shader;
	}
	void RayTracingPipeline::create_create_info()
	{
		if (m_created)
			destroy();

		set_default_create_info();
		set_shader_stages();

		m_createInfo.layout = m_layout->m_layout;
	}
	void RayTracingPipeline::create()
	{
		auto res = vkCreateRayTracingPipelinesKHR(
			m_device,
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			1,
			&m_createInfo,
			nullptr,
			&m_pipeline
		);

		VK_CHECK_ERROR(res)
	}

	void RayTracingPipeline::set_default_create_info()
	{
		m_createInfo = {};

		m_createInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		m_createInfo.flags = 0;
		m_createInfo.pNext = nullptr;

		// ---------------------------------------

		m_createInfo.basePipelineHandle = VK_NULL_HANDLE;
		m_createInfo.basePipelineIndex = -1;

		m_createInfo.layout = *m_layout;

		// ---------------------------------------

		m_creationData.dynamicStates = {
		   VK_DYNAMIC_STATE_VIEWPORT,
		   VK_DYNAMIC_STATE_SCISSOR
		};
		m_creationData.dynamicState = {};
		m_creationData.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		m_creationData.dynamicState.dynamicStateCount = static_cast<uint32_t>(m_creationData.dynamicStates.size());
		m_creationData.dynamicState.pDynamicStates = m_creationData.dynamicStates.data();

		m_createInfo.pDynamicState = &m_creationData.dynamicState;

		// ---------------------------------------

		m_createInfo.pLibraryInfo = nullptr;
		m_createInfo.pLibraryInterface = nullptr;

		// ---------------------------------------

		m_createInfo.maxPipelineRayRecursionDepth = 1;
	}
	void RayTracingPipeline::set_shader_stages()
	{
		// ray generation
		{
			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.flags = 0;
			shaderStageCI.pNext = nullptr;
			shaderStageCI.module = m_shader->raygen.m_module;
			shaderStageCI.pName = "main";
			shaderStageCI.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

			VkRayTracingShaderGroupCreateInfoKHR groupCI = {};
			groupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupCI.pNext = nullptr;
			groupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groupCI.generalShader = static_cast<uint32_t>(m_creationData.groupCIs.size()) - 1;
			groupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
			groupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
			groupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

			m_creationData.stageCIs.push_back(shaderStageCI);
			m_creationData.groupCIs.push_back(groupCI);
		}

		// miss
		{
			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.flags = 0;
			shaderStageCI.pNext = nullptr;
			shaderStageCI.module = m_shader->miss.m_module;
			shaderStageCI.pName = "main";
			shaderStageCI.stage = VK_SHADER_STAGE_MISS_BIT_KHR;

			VkRayTracingShaderGroupCreateInfoKHR groupCI = {};
			groupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupCI.pNext = nullptr;
			groupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groupCI.generalShader = static_cast<uint32_t>(m_creationData.groupCIs.size()) - 1;
			groupCI.closestHitShader = VK_SHADER_UNUSED_KHR;
			groupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
			groupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

			m_creationData.stageCIs.push_back(shaderStageCI);
			m_creationData.groupCIs.push_back(groupCI);
		}

		// hit
		{
			VkPipelineShaderStageCreateInfo shaderStageCI = {};
			shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCI.flags = 0;
			shaderStageCI.pNext = nullptr;
			shaderStageCI.module = m_shader->hit.m_module;
			shaderStageCI.pName = "main";
			shaderStageCI.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

			VkRayTracingShaderGroupCreateInfoKHR groupCI = {};
			groupCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			groupCI.pNext = nullptr;
			groupCI.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			groupCI.generalShader = VK_SHADER_UNUSED_KHR;
			groupCI.closestHitShader = static_cast<uint32_t>(m_creationData.groupCIs.size()) - 1;
			groupCI.anyHitShader = VK_SHADER_UNUSED_KHR;
			groupCI.intersectionShader = VK_SHADER_UNUSED_KHR;

			m_creationData.stageCIs.push_back(shaderStageCI);
			m_creationData.groupCIs.push_back(groupCI);
		}

		m_createInfo.pStages = m_creationData.stageCIs.data();
		m_createInfo.stageCount = static_cast<uint32_t>(m_creationData.stageCIs.size());

		m_createInfo.pGroups = m_creationData.groupCIs.data();
		m_createInfo.groupCount = static_cast<uint32_t>(m_creationData.groupCIs.size());
	}

	// --------------------------------------
	// PIPELINE BATCH CREATOR
	// --------------------------------------

	PipelineBatchCreator::PipelineBatchCreator() :
		m_cacheCreated{ false }
	{}

	void PipelineBatchCreator::schedule_creation(
		PipelineRef pipeline
	)
	{
		m_pipelines.push_back(pipeline);
	}
	void PipelineBatchCreator::create_all()
	{
		create_cache();

		create_graphics_pipelines();
		create_compute_pipelines();
		create_raytracing_pipelines();

		for (auto pipeline : m_pipelines) pipeline->set_created();
		m_pipelines.clear();
	}
	void PipelineBatchCreator::destroy()
	{
		vkDestroyPipelineCache(
			m_device,
			m_cache,
			nullptr
		);
	}

	// --------------------------------------
	// PRIVATE
	// --------------------------------------

	void PipelineBatchCreator::create_cache()
	{
		if (m_cacheCreated || m_pipelines.empty())
			return;

		m_device = m_pipelines.front()->m_device;

		VkPipelineCacheCreateInfo cacheCI = {};
		cacheCI.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		cacheCI.pNext = nullptr;
		cacheCI.flags = 0;

		cacheCI.initialDataSize = 0;
		cacheCI.pInitialData = nullptr;

		vkCreatePipelineCache(m_device, &cacheCI, nullptr, &m_cache);

		m_cacheCreated = true;
	}

	// stores all pipelines of the given type (e.g. graphics, compute, ...) into `pipelines`
	template<typename P> void get_specific_pipelines(const std::vector<PipelineRef>& allPipelines, std::vector<P*>& pipelines, PipelineType type)
	{
		for (auto pipeline : allPipelines)
		{
			if (pipeline->m_type == type)
			{
				auto specificPipeline = dynamic_cast<P*>(pipeline.get());
				pipelines.push_back(specificPipeline);
			}
		}
	}

	// sorts the given pipelines into buckets without destroying the already given buckets
	// returns the start of the newly sorted data (the old data is in front of it)
	template<typename T> size_t sort_into_buckets(std::vector<std::vector<GraphicsPipeline*>>& partitionedPipelines, size_t startIndex, std::function<T(GraphicsPipeline*)> accessor)
	{
		std::unordered_map<T, std::vector<GraphicsPipeline*>> mappedPipelines;
		size_t endIndex = partitionedPipelines.size();
		for (size_t i = startIndex; i < endIndex; i++)
		{
			mappedPipelines.clear();
			for (auto pipeline : partitionedPipelines[i])
				mappedPipelines[accessor(pipeline)].push_back(pipeline);
			for (auto [device, p] : mappedPipelines)
				partitionedPipelines.push_back(p);
		}
		return endIndex;
	}

	// creates the pipelines of a specific type (graphics, compute, ...) given with `T`
	// `C` is the corresponding VkCreateInfo type
	// `createPipelines` contains the vulkan function to actually create the pipelines (e.g. vkCreateGraphicsPipelines(...))
	template<typename T, typename C>
	void create_specific_pipelines(
		std::vector<PipelineRef>& m_pipelines, VkPipelineCache& m_cache,
		std::function<void(VkPipelineCache, const std::vector<C>&, std::vector<VkPipeline>&)> createPipelines
	)
	{
		std::vector<std::vector<T*>> partitionedPipelines = { std::vector<T*>() };

		get_specific_pipelines(m_pipelines, partitionedPipelines.front(), GraphicsPipeline_E);

		size_t start = 0;

		start = sort_into_buckets<PipelineLayoutRef>(partitionedPipelines, start, [](T* pipeline) { return pipeline->m_layout; });

		partitionedPipelines.erase(
			partitionedPipelines.begin(),
			partitionedPipelines.begin() + start
		);

		for (const auto& pipelines : partitionedPipelines)
		{
			std::vector<VkPipeline> vulkanPipelines(pipelines.size());
			std::vector<C> createInfos(pipelines.size());
			for (size_t i = 0; i < pipelines.size(); i++)
			{
				vulkanPipelines[i] = pipelines[i]->m_pipeline;
				createInfos[i] = pipelines[i]->m_createInfo;
			}

			createPipelines(
				m_cache,
				createInfos,
				vulkanPipelines
			);

			for (size_t i = 0; i < pipelines.size(); i++)
				pipelines[i]->m_pipeline = vulkanPipelines[i];
		}
	}

	void PipelineBatchCreator::create_graphics_pipelines()
	{
		create_specific_pipelines<GraphicsPipeline, VkGraphicsPipelineCreateInfo>(
			m_pipelines, m_cache,
			[this](
				VkPipelineCache cache,
				const std::vector<VkGraphicsPipelineCreateInfo>& createInfos,
				std::vector<VkPipeline>& pipelines
				)
			{
				vkCreateGraphicsPipelines(
					m_device,
					cache,
					createInfos.size(),
					createInfos.data(),
					nullptr,
					pipelines.data()
				);
			}
		);
	}
	void PipelineBatchCreator::create_compute_pipelines()
	{

	}
	void PipelineBatchCreator::create_raytracing_pipelines()
	{

	}

} // namespace vk
