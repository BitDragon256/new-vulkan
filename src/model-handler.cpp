#include "model-handler.h"

#include "math-core.h"

// ------------------------------------------
// NON-MEMBER FUNCTIONS
// ------------------------------------------

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
VkCommandBufferBeginInfo create_command_buffer_begin_info(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer, VkCommandBufferInheritanceInfo& inheritanceI)
{
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
	commandBufferBI.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
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

// ------------------------------------------
// STATIC GEOMETRY HANDLER
// ------------------------------------------

void StaticGeometryHandler::initialize(StaticGeometryHandlerVulkanObjects vulkanObjects)
{
	m_vulkanObjects = vulkanObjects;
	reloadMeshBuffers = true;

	create_descriptor_set();
}
void StaticGeometryHandler::update_framebuffers(std::vector<VkFramebuffer> framebuffers, VkExtent2D swapchainExtent)
{
	m_vulkanObjects.framebuffers = framebuffers;
	m_vulkanObjects.swapchainExtent = swapchainExtent;
}

void StaticGeometryHandler::awake(EntityId entity, ECSManager& ecs)
{
	Transform& transform = ecs.get_component<Transform>(entity);
	StaticMesh& mesh = ecs.get_component<StaticMesh>(entity);

	add_mesh(mesh, transform);
}
void StaticGeometryHandler::update(float dt, ECSManager& ecs)
{
	if (reloadMeshBuffers)
	{
		reload_meshes();
		reloadMeshBuffers = false;
	}
}

void StaticGeometryHandler::add_mesh(StaticMesh& mesh, Transform transform)
{
	bake_transform(mesh, transform);
	size_t index;
	MeshGroup* meshGroup = find_group(mesh.material, index);
	if (!meshGroup) // no group found
		meshGroup = push_mesh_group(mesh.material);

	m_meshes.push_back(MeshDataInfo{ meshGroup->vertices.size(), mesh.vertices.size(), meshGroup->indices.size(), mesh.indices.size(), index });
	append_vector(meshGroup->vertices, mesh.vertices);
	append_vector(meshGroup->indices, mesh.indices);

	meshGroup->reloadMeshBuffers = true;
	reloadMeshBuffers = true;
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
MeshGroup* StaticGeometryHandler::push_mesh_group(const Material& material)
{
	m_meshGroups.push_back(MeshGroup {});
	auto& meshGroup = m_meshGroups.back();
	meshGroup.material = material;

	BufferConfig config = default_buffer_config();
	config.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	meshGroup.vertexBuffer.initialize(config);

	config.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	meshGroup.indexBuffer.initialize(config);

	create_group_command_buffers(meshGroup);

	return &meshGroup;
}

//void StaticGeometryHandler::create_command_buffers()
//{
//	for (MeshGroup& meshGroup : m_meshGroups)
//		for (VkCommandBuffer& commandBuffer : meshGroup.commandBuffers)
//			create_command_buffer(&commandBuffer);
//}
void StaticGeometryHandler::create_group_command_buffers(MeshGroup& meshGroup)
{
	meshGroup.commandBuffers.resize(m_vulkanObjects.framebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAI = {};
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.pNext = nullptr;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	commandBufferAI.commandPool = m_vulkanObjects.commandPool;
	commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_vulkanObjects.framebuffers.size());
	auto res = vkAllocateCommandBuffers(m_vulkanObjects.device, &commandBufferAI, meshGroup.commandBuffers.data());
	log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer for the static geometry handler");
}
void StaticGeometryHandler::record_command_buffers(uint32_t frame)
{
	// record command buffers
	uint32_t subpass = m_vulkanObjects.firstSubpass;
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		record_command_buffer(subpass, frame, meshGroup);
		subpass++;
	}
}
void StaticGeometryHandler::record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup)
{
	VkCommandBuffer commandBuffer = meshGroup.commandBuffers[frame];
	vkResetCommandBuffer(commandBuffer, 0);

	// ---------------------------------------

	VkCommandBufferInheritanceInfo inheritanceInfo;
	VkCommandBufferBeginInfo commandBufferBI = create_command_buffer_begin_info(m_vulkanObjects.renderPass, subpass, m_vulkanObjects.framebuffers[static_cast<size_t>(frame)], inheritanceInfo);

	{
		auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
		log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording for the static geometry handler");
	}

	// ---------------------------------------

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshGroup.pipeline);
	
	// ---------------------------------------

	set_dynamic_state(commandBuffer, m_vulkanObjects.swapchainExtent);

	// ---------------------------------------

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

	// --------------------------------------

	vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(CameraPushConstant), m_vulkanObjects.pCameraPushConstant);

	// ---------------------------------------

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshGroup.vertexBuffer.m_buffer, offsets);

	// ---------------------------------------

	vkCmdBindIndexBuffer(commandBuffer, meshGroup.indexBuffer.m_buffer, 0, NVE_INDEX_TYPE);

	// ---------------------------------------

	vkCmdDrawIndexed(commandBuffer, meshGroup.indices.size(), 1, 0, 0, 0);

	// ---------------------------------------

	{
		auto res = vkEndCommandBuffer(commandBuffer);
		log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording for the static geometry handler");
	}

	// ---------------------------------------
}
std::vector<VkCommandBuffer> StaticGeometryHandler::get_command_buffers(uint32_t frame)
{
	std::vector<VkCommandBuffer> buffers;

	for (MeshGroup& meshGroup : m_meshGroups)
		buffers.push_back(meshGroup.commandBuffers[static_cast<size_t>(frame)]);

	return buffers;
}

void StaticGeometryHandler::create_pipeline_create_infos(std::vector<VkGraphicsPipelineCreateInfo>& createInfos)
{
	m_pipelineCreationData.resize(m_vulkanObjects.framebuffers.size());

	size_t index = 0;
	uint32_t subpass = m_vulkanObjects.firstSubpass;
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		createInfos.push_back(create_pipeline_create_info(subpass, index));

		index++;
		subpass++;
	}
}
void StaticGeometryHandler::set_pipelines(std::vector<VkPipeline>& pipelines)
{
	for (size_t i = 0; i < pipelines.size(); i++)
		m_meshGroups[i].pipeline = pipelines[i];
}

VkGraphicsPipelineCreateInfo StaticGeometryHandler::create_pipeline_create_info(uint32_t subpass, size_t pipelineIndex)
{
	PipelineCreationData& pipelineCreationData = m_pipelineCreationData[pipelineIndex];

	// ---------------------------------------

	VkGraphicsPipelineCreateInfo graphicsPipelineCI = create_default_pipeline_create_info(pipelineCreationData);

	// ---------------------------------------

	create_pipeline_shader_stages(graphicsPipelineCI, pipelineCreationData, m_meshGroups[pipelineIndex].material);

	// ---------------------------------------

	create_pipeline_layout();
	graphicsPipelineCI.layout = m_pipelineLayout;

	// ---------------------------------------

	graphicsPipelineCI.renderPass = m_vulkanObjects.renderPass;
	graphicsPipelineCI.subpass = subpass;
	graphicsPipelineCI.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCI.basePipelineIndex = -1;

	// ---------------------------------------

	return graphicsPipelineCI;
}

void StaticGeometryHandler::create_pipeline_layout()
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

	pipelineLayoutCI.pSetLayouts = &m_descriptorSetLayout;
	pipelineLayoutCI.setLayoutCount = 1;

	{
		auto res = vkCreatePipelineLayout(m_vulkanObjects.device, &pipelineLayoutCI, nullptr, &m_pipelineLayout);
		log_cond_err(res == VK_SUCCESS, "failed to create basic pipeline layout");
	}
}

void StaticGeometryHandler::reload_meshes()
{
	for (MeshGroup& meshGroup : m_meshGroups)
		if (meshGroup.reloadMeshBuffers)
			reload_mesh_group(meshGroup);
	reloadMeshBuffers = false;
}
void StaticGeometryHandler::reload_mesh_group(MeshGroup& meshGroup)
{
	meshGroup.vertexBuffer.set(meshGroup.vertices);
	meshGroup.indexBuffer.set(meshGroup.indices);

	meshGroup.reloadMeshBuffers = false;
}
BufferConfig StaticGeometryHandler::default_buffer_config()
{
	BufferConfig config = {};
	config.device = m_vulkanObjects.device;
	config.physicalDevice = m_vulkanObjects.physicalDevice;
	config.stagedBufferTransferCommandPool = m_vulkanObjects.commandPool;
	config.stagedBufferTransferQueue = m_vulkanObjects.transferQueue;

	config.useStagedBuffer = true;
	config.singleUseStagedBuffer = true;
	config.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	return config;
} 

uint32_t StaticGeometryHandler::subpass_count()
{
	return 1;
}

void StaticGeometryHandler::create_descriptor_set()
{
	VkDescriptorSetLayoutBinding texturePoolBinding = {};
	texturePoolBinding.descriptorCount = TEXTURE_POOL_MAX_TEXTURES;
	texturePoolBinding.binding = STATIC_GEOMETRY_HANDLER_TEXTURE_BINDING;
	texturePoolBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texturePoolBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texturePoolBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
		texturePoolBinding
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pNext = nullptr;
	descriptorSetLayoutCI.flags = 0;
	descriptorSetLayoutCI.bindingCount = layoutBindings.size();
	descriptorSetLayoutCI.pBindings = layoutBindings.data();

	{
		auto res = vkCreateDescriptorSetLayout(m_vulkanObjects.device, &descriptorSetLayoutCI, nullptr, &m_descriptorSetLayout);
		log_cond_err(res == VK_SUCCESS, "failed to create static geometry handler descriptor set layout");
	}

	// ----------------------------------------------------

	VkDescriptorSetAllocateInfo descriptorSetAI = {};
	descriptorSetAI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAI.pNext = nullptr;
	descriptorSetAI.descriptorPool = m_vulkanObjects.descriptorPool;
	descriptorSetAI.descriptorSetCount = 1;
	descriptorSetAI.pSetLayouts = &m_descriptorSetLayout;

	{
		auto res = vkAllocateDescriptorSets(m_vulkanObjects.device, &descriptorSetAI, &m_descriptorSet);
		log_cond_err(res == VK_SUCCESS, "failed to allocate static geometry handler descriptor set");
	}
}
void StaticGeometryHandler::update_descriptor_set()
{
	std::vector<VkWriteDescriptorSet> writes;

	VkWriteDescriptorSet textureWrite = m_texturePool.get_descriptor_set_write(m_descriptorSet, STATIC_GEOMETRY_HANDLER_TEXTURE_BINDING);
	writes.push_back(textureWrite);

	if (writes.size() > 0)
		vkUpdateDescriptorSets(m_vulkanObjects.device, writes.size(), writes.data(), 0, nullptr);
}