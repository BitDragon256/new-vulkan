#include "model-handler.h"

#include "math-core.h"

template<typename T>
void append_vector(std::vector<T>& origin, std::vector<T>& appendage)
{
	origin.insert(origin.end(), appendage.begin(), appendage.end());
}
void bake_transform(StaticMesh& mesh, Transform transform)
{
	for (Vertex& vertex : mesh.vertices)
	{
		vertex.pos = Quaternion::rotate(vertex.pos * transform.scale, transform.rotation) + transform.position;
	}
}

void set_dynamic_state(VkCommandBuffer commandBuffer, VkExtent2D swapChainExtent)
{
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}
VkCommandBufferBeginInfo create_command_buffer_begin_info(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer)
{
	VkCommandBufferInheritanceInfo inheritanceI = {};
	inheritanceI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritanceI.pNext = nullptr;
	inheritanceI.renderPass = renderPass;
	inheritanceI.subpass = subpass;
	inheritanceI.framebuffer = framebuffer;
	inheritanceI.occlusionQueryEnable = VK_FALSE;
	inheritanceI.queryFlags = 0;
	inheritanceI.pipelineStatistics = 0;

	VkCommandBufferBeginInfo commandBufferBI = {};
	commandBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBI.pNext = nullptr;
	commandBufferBI.flags = 0;
	commandBufferBI.pInheritanceInfo = &inheritanceI;

	return commandBufferBI;
}

VkGraphicsPipelineCreateInfo create_default_pipeline_create_info(PipelineCreationData& pipelineCreationData)
{
	VkGraphicsPipelineCreateInfo graphicsPipelineCI = {};
	graphicsPipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCI.pNext = nullptr;
	graphicsPipelineCI.flags = 0;

	// ---------------------------------------

	pipelineCreationData.vertexInputState = {};
	pipelineCreationData.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineCreationData.vertexInputState.pNext = nullptr;
	pipelineCreationData.vertexInputState.flags = 0;

	pipelineCreationData.vertexAttributeDescriptions = Vertex::getAttributeDescriptions();
	pipelineCreationData.vertexInputState.vertexAttributeDescriptionCount = pipelineCreationData.vertexAttributeDescriptions.size();
	pipelineCreationData.vertexInputState.pVertexAttributeDescriptions = pipelineCreationData.vertexAttributeDescriptions.data();

	pipelineCreationData.vertexBindingDescription = Vertex::getBindingDescription();
	pipelineCreationData.vertexInputState.vertexBindingDescriptionCount = 1;
	pipelineCreationData.vertexInputState.pVertexBindingDescriptions = &pipelineCreationData.vertexBindingDescription;

	graphicsPipelineCI.pVertexInputState = &pipelineCreationData.vertexInputState;

	// -------------------------------------------

	pipelineCreationData.inputAssemblyState = {};
	pipelineCreationData.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineCreationData.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreationData.inputAssemblyState.primitiveRestartEnable = VK_FALSE;

	graphicsPipelineCI.pInputAssemblyState = &pipelineCreationData.inputAssemblyState;

	// -------------------------------------------

	pipelineCreationData.viewportState = {};
	pipelineCreationData.viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineCreationData.viewportState.viewportCount = 1;
	pipelineCreationData.viewportState.scissorCount = 1;

	graphicsPipelineCI.pViewportState = &pipelineCreationData.viewportState;

	// -------------------------------------------

	pipelineCreationData.rasterizationState = {};
	pipelineCreationData.rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineCreationData.rasterizationState.depthClampEnable = VK_FALSE;
	pipelineCreationData.rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	pipelineCreationData.rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineCreationData.rasterizationState.lineWidth = 1.0f;
	pipelineCreationData.rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineCreationData.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineCreationData.rasterizationState.depthBiasEnable = VK_FALSE;

	graphicsPipelineCI.pRasterizationState = &pipelineCreationData.rasterizationState;

	// -------------------------------------------

	pipelineCreationData.multisampleState = {};
	pipelineCreationData.multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineCreationData.multisampleState.sampleShadingEnable = VK_FALSE;
	pipelineCreationData.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	graphicsPipelineCI.pMultisampleState = &pipelineCreationData.multisampleState;

	// -------------------------------------------

	pipelineCreationData.colorBlendAttachment = {};
	pipelineCreationData.colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineCreationData.colorBlendAttachment.blendEnable = VK_FALSE;

	pipelineCreationData.colorBlendState = {};
	pipelineCreationData.colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineCreationData.colorBlendState.logicOpEnable = VK_FALSE;
	pipelineCreationData.colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	pipelineCreationData.colorBlendState.attachmentCount = 1;
	pipelineCreationData.colorBlendState.pAttachments = &pipelineCreationData.colorBlendAttachment;
	pipelineCreationData.colorBlendState.blendConstants[0] = 0.0f;
	pipelineCreationData.colorBlendState.blendConstants[1] = 0.0f;
	pipelineCreationData.colorBlendState.blendConstants[2] = 0.0f;
	pipelineCreationData.colorBlendState.blendConstants[3] = 0.0f;

	graphicsPipelineCI.pColorBlendState = &pipelineCreationData.colorBlendState;

	// -------------------------------------------
	pipelineCreationData.dynamicStates = {
	   VK_DYNAMIC_STATE_VIEWPORT,
	   VK_DYNAMIC_STATE_SCISSOR
	};
	pipelineCreationData.dynamicState = {};
	pipelineCreationData.dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineCreationData.dynamicState.dynamicStateCount = static_cast<uint32_t>(pipelineCreationData.dynamicStates.size());
	pipelineCreationData.dynamicState.pDynamicStates = pipelineCreationData.dynamicStates.data();

	graphicsPipelineCI.pDynamicState = &pipelineCreationData.dynamicState;

	// -------------------------------------------

	return graphicsPipelineCI;
}

void StaticGeometryHandler::awake(EntityId entity, ECSManager& ecs)
{
	Transform& transform = ecs.get_component<Transform>(entity);
	StaticMesh& mesh = ecs.get_component<StaticMesh>(entity);

	add_mesh(mesh, transform);
}

void StaticGeometryHandler::add_mesh(StaticMesh& mesh, Transform transform)
{
	bake_transform(mesh, transform);
	size_t index;
	MeshGroup* group = find_group(mesh.material, index);
	if (!group) // no group found
	{
		m_meshGroups.push_back(MeshGroup {});
		group = &m_meshGroups.back();
		group->material = mesh.material;
	}
	m_meshes.push_back(MeshDataInfo{ group->vertices.size(), mesh.vertices.size(), group->indices.size(), mesh.indices.size(), index });
	append_vector(group->vertices, mesh.vertices);
	append_vector(group->indices, mesh.indices);
}
MeshGroup* StaticGeometryHandler::find_group(const Material& material, size_t& index)
{
	while (index < m_meshGroups.size())
	{
		if (m_meshGroups[index].material == material)
			return &m_meshGroups[index];
		index++;
	}
	return nullptr;
}
void StaticGeometryHandler::create_command_buffers(VkDevice device, VkCommandPool commandPool)
{
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		create_command_buffer(device, commandPool, &meshGroup.commandBuffer);
	}
}
void StaticGeometryHandler::create_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer* pCommandBuffer)
{
	VkCommandBufferAllocateInfo commandBufferAI = {};
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.pNext = nullptr;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	commandBufferAI.commandPool = commandPool;
	commandBufferAI.commandBufferCount = 1;
	auto res = vkAllocateCommandBuffers(device, &commandBufferAI, pCommandBuffer);
	log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer for the static geometry handler");
}

void StaticGeometryHandler::record_command_buffers(VkRenderPass renderPass, uint32_t firstSubpass, VkFramebuffer framebuffer, VkExtent2D swapchainExtent)
{
	uint32_t subpass = firstSubpass;
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		record_command_buffer(renderPass, subpass, framebuffer, swapchainExtent, meshGroup);

		subpass++;
	}
}
void StaticGeometryHandler::record_command_buffer(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkExtent2D swapChainExtent, const MeshGroup& meshGroup)
{
	// ---------------------------------------

	VkCommandBufferBeginInfo commandBufferBI = create_command_buffer_begin_info(renderPass, subpass, framebuffer);

	{
		auto res = vkBeginCommandBuffer(meshGroup.commandBuffer, &commandBufferBI);
		log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording for the static geometry handler");
	}

	// ---------------------------------------

	vkCmdBindPipeline(meshGroup.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshGroup.pipeline);
	
	// ---------------------------------------

	set_dynamic_state(meshGroup.commandBuffer, swapChainExtent);

	// ---------------------------------------

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(meshGroup.commandBuffer, 0, 1, &meshGroup.m_vertexBuffer.m_buffer, offsets);

	// ---------------------------------------

	vkCmdBindIndexBuffer(meshGroup.commandBuffer, meshGroup.m_indexBuffer.m_buffer, 0, NVE_INDEX_TYPE);

	// ---------------------------------------

	vkCmdDrawIndexed(meshGroup.commandBuffer, meshGroup.indices.size(), 1, 0, 0, 0);

	// ---------------------------------------

	{
		auto res = vkEndCommandBuffer(meshGroup.commandBuffer);
		log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording for the static geometry handler");
	}

	// ---------------------------------------
}

void StaticGeometryHandler::create_pipelines(VkDevice device, VkRenderPass renderPass, uint32_t firstSubpass, std::vector<VkPipeline>& pipelines, std::vector<VkGraphicsPipelineCreateInfo>& createInfos)
{
	size_t index = 0;
	uint32_t subpass = firstSubpass;
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		createInfos.push_back(create_pipeline_create_info(device, renderPass, subpass, index));
		pipelines.push_back(meshGroup.pipeline);

		index++;
		subpass++;
	}
}

void create_pipeline_shader_stages(VkGraphicsPipelineCreateInfo& graphicsPipelineCI, PipelineCreationData& pipelineCreationData, const Material& material)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = *material.m_pShader;
	vertShaderStageInfo.pName = "vert";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = *material.m_pShader;
	fragShaderStageInfo.pName = "frag";

	pipelineCreationData.stages[0] = vertShaderStageInfo;
	pipelineCreationData.stages[1] = fragShaderStageInfo;

	graphicsPipelineCI.stageCount = 2;
	graphicsPipelineCI.pStages = pipelineCreationData.stages;
}

VkGraphicsPipelineCreateInfo StaticGeometryHandler::create_pipeline_create_info(VkDevice device, VkRenderPass renderPass, uint32_t subpass, size_t pipelineIndex)
{
	PipelineCreationData& pipelineCreationData = m_pipelineCreationData[pipelineIndex];

	// ---------------------------------------

	VkGraphicsPipelineCreateInfo graphicsPipelineCI = create_default_pipeline_create_info(pipelineCreationData);

	// ---------------------------------------

	create_pipeline_shader_stages(graphicsPipelineCI, pipelineCreationData, m_meshGroups[pipelineIndex].material);

	// ---------------------------------------

	create_pipeline_layout(device);
	graphicsPipelineCI.layout = m_pipelineLayout;

	// ---------------------------------------

	graphicsPipelineCI.renderPass = renderPass;
	graphicsPipelineCI.subpass = subpass;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCI.basePipelineIndex = -1;

	// ---------------------------------------

	return graphicsPipelineCI;
}

void StaticGeometryHandler::create_pipeline_layout(VkDevice device)
{
	VkPipelineLayoutCreateInfo pipelineLayoutCI = {};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.pNext = nullptr;
	pipelineLayoutCI.flags = 0;

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(CameraPushConstant);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
	pipelineLayoutCI.pushConstantRangeCount = 1;

	pipelineLayoutCI.pSetLayouts = nullptr;
	pipelineLayoutCI.setLayoutCount = 0;

	{
		auto res = vkCreatePipelineLayout(device, &pipelineLayoutCI, nullptr, &m_pipelineLayout);
		log_cond_err(res == VK_SUCCESS, "failed to create basic pipeline layout");
	}
}