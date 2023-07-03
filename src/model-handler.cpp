#include "model-handler.h"

#include <set>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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
	VkViewport viewport = {};
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
	pipelineCreationData.vertexInputState.vertexAttributeDescriptionCount = 0;
	pipelineCreationData.vertexInputState.vertexAttributeDescriptionCount = pipelineCreationData.vertexAttributeDescriptions.size();
	pipelineCreationData.vertexInputState.pVertexAttributeDescriptions = pipelineCreationData.vertexAttributeDescriptions.data();

	pipelineCreationData.vertexBindingDescription = Vertex::getBindingDescription();
	pipelineCreationData.vertexInputState.vertexBindingDescriptionCount = 0;
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
	pipelineCreationData.rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	pipelineCreationData.rasterizationState.depthBiasEnable = VK_FALSE;

	graphicsPipelineCI.pRasterizationState = &pipelineCreationData.rasterizationState;

	// -------------------------------------------

	pipelineCreationData.multisampleState = {};
	pipelineCreationData.multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineCreationData.multisampleState.sampleShadingEnable = VK_FALSE;
	pipelineCreationData.multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	graphicsPipelineCI.pMultisampleState = &pipelineCreationData.multisampleState;

	// -------------------------------------------
	
	pipelineCreationData.depthStencilState = {};
	pipelineCreationData.depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineCreationData.depthStencilState.pNext = nullptr;
	pipelineCreationData.depthStencilState.flags = 0;
	pipelineCreationData.depthStencilState.depthTestEnable = VK_TRUE;
	pipelineCreationData.depthStencilState.depthWriteEnable = VK_TRUE;
	pipelineCreationData.depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
	pipelineCreationData.depthStencilState.depthBoundsTestEnable = VK_FALSE;
	pipelineCreationData.depthStencilState.stencilTestEnable = VK_FALSE;

	graphicsPipelineCI.pDepthStencilState = &pipelineCreationData.depthStencilState;

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

void create_pipeline_shader_stages(VkGraphicsPipelineCreateInfo& graphicsPipelineCI, PipelineCreationData& pipelineCreationData, const GraphicsShader& shader)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = shader.vertex.m_module;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = shader.fragment.m_module;
	fragShaderStageInfo.pName = "main";

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

	BufferConfig matBufConf = default_buffer_config();
	matBufConf.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_materialBuffer.initialize(matBufConf);

	m_texturePool.init(m_vulkanObjects.device, m_vulkanObjects.physicalDevice, m_vulkanObjects.commandPool, m_vulkanObjects.transferQueue);

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
	StaticModel& model = ecs.get_component<StaticModel>(entity);

	add_model(model, transform);
}
void StaticGeometryHandler::update(float dt, ECSManager& ecs)
{
	if (reloadMeshBuffers)
	{
		reload_meshes();
		reloadMeshBuffers = false;
	}
}

void StaticGeometryHandler::add_model(StaticModel& model, Transform transform)
{
	for (auto& mesh : model.m_children)
	{
		bake_transform(mesh, transform);
		size_t index;
		MeshGroup* meshGroup = find_group(mesh.material.m_shader, index);
		if (!meshGroup) // no group found
			meshGroup = push_mesh_group(mesh.material.m_shader);

		// material
		uint32_t materialIndex = static_cast<uint32_t>(std::find(m_materials.begin(), m_materials.end(), mesh.material) - m_materials.begin());
		if (materialIndex == m_materials.size())
			m_materials.push_back(mesh.material);
		for (auto& vert : mesh.vertices)
			vert.material = materialIndex;

		// save mesh
		m_meshes.push_back(MeshDataInfo{ meshGroup->vertices.size(), mesh.vertices.size(), meshGroup->indices.size(), mesh.indices.size(), index });
		append_vector(meshGroup->vertices, mesh.vertices);
		append_vector(meshGroup->indices, mesh.indices);

		meshGroup->reloadMeshBuffers = true;
		reloadMeshBuffers = true;
	}
}
MeshGroup* StaticGeometryHandler::find_group(GraphicsShader*& shader, size_t& index)
{
	if (!shader)
	{
		// automatically assign the first best
		if (m_meshGroups.size() > 0)
		{
			shader = &m_meshGroups[0].shader;
			return &m_meshGroups[0];
		}
		return nullptr;
	}

	index = 0;
	while (index < m_meshGroups.size())
	{
		if (m_meshGroups[index].shader == (*shader))
			return &m_meshGroups[index];
		index++;
	}
	return nullptr;
}
MeshGroup* StaticGeometryHandler::push_mesh_group(GraphicsShader*& shader)
{
	m_meshGroups.push_back(MeshGroup {});
	auto& meshGroup = m_meshGroups.back();
	meshGroup.shader.set_default_shader();
	shader = &meshGroup.shader;

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

	vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CameraPushConstant), m_vulkanObjects.pCameraPushConstant);

	// ---------------------------------------

	VkDeviceSize offsets[1] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &meshGroup.vertexBuffer.m_buffer, offsets);

	// ---------------------------------------

	vkCmdBindIndexBuffer(commandBuffer, meshGroup.indexBuffer.m_buffer, 0, NVE_INDEX_TYPE);

	// ---------------------------------------

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshGroup.indices.size()), 1, 0, 0, 0);

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
	m_pipelineCreationData.resize(m_meshGroups.size());

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

	create_pipeline_shader_stages(graphicsPipelineCI, pipelineCreationData, m_meshGroups[pipelineIndex].shader);

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
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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
	// reload materials
	std::vector<MaterialSSBO> mats(m_materials.size());
	for (size_t i = 0; i < mats.size(); i++)
	{
		mats[i] = m_materials[i];
		if (!m_materials[i].m_diffuseTex.empty())
			mats[i].m_textureIndex = m_texturePool.find(m_materials[i].m_diffuseTex);
		else
			mats[i].m_textureIndex = UINT32_MAX;
	}
	m_materialBuffer.set(mats);
	update_descriptor_set();

	// reload meshes
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

VkWriteDescriptorSet StaticGeometryHandler::material_buffer_descriptor_set_write()
{
	m_materialBufferDescriptorInfo = {};
	m_materialBufferDescriptorInfo.buffer = m_materialBuffer.m_buffer;
	m_materialBufferDescriptorInfo.offset = 0;
	m_materialBufferDescriptorInfo.range = sizeof(MaterialSSBO) * m_materials.size();

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSet;
	write.dstBinding = STATIC_GEOMETRY_HANDLER_MATERIAL_BINDING;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &m_materialBufferDescriptorInfo;

	return write;
}

void StaticGeometryHandler::create_descriptor_set()
{
	VkDescriptorSetLayoutBinding materialBufferBinding = {};
	materialBufferBinding.descriptorCount = 1;
	materialBufferBinding.binding = STATIC_GEOMETRY_HANDLER_MATERIAL_BINDING;
	materialBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	materialBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	materialBufferBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding texturePoolBinding = {};
	texturePoolBinding.descriptorCount = TEXTURE_POOL_MAX_TEXTURES;
	texturePoolBinding.binding = STATIC_GEOMETRY_HANDLER_TEXTURE_BINDING;
	texturePoolBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texturePoolBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texturePoolBinding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.descriptorCount = 1;
	samplerBinding.binding = STATIC_GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
		materialBufferBinding,
		texturePoolBinding,
		samplerBinding,
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

	writes.push_back(material_buffer_descriptor_set_write());
	writes.push_back(m_texturePool.get_descriptor_set_write(m_descriptorSet, STATIC_GEOMETRY_HANDLER_TEXTURE_BINDING));
	writes.push_back(m_texturePool.get_sampler_descriptor_set_write(m_descriptorSet, STATIC_GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING));

	vkUpdateDescriptorSets(m_vulkanObjects.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

void StaticGeometryHandler::cleanup()
{
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		meshGroup.indexBuffer.destroy();
		meshGroup.vertexBuffer.destroy();

		m_materialBuffer.destroy();

		vkDestroyPipeline(m_vulkanObjects.device, meshGroup.pipeline, nullptr);
		vkDestroyPipelineLayout(m_vulkanObjects.device, m_pipelineLayout, nullptr);

		vkFreeDescriptorSets(m_vulkanObjects.device, m_vulkanObjects.descriptorPool, 1, &m_descriptorSet);
		vkDestroyDescriptorSetLayout(m_vulkanObjects.device, m_descriptorSetLayout, nullptr);

		vkFreeCommandBuffers(m_vulkanObjects.device, m_vulkanObjects.commandPool, meshGroup.commandBuffers.size(), meshGroup.commandBuffers.data());

		meshGroup.shader.fragment.destroy();
		meshGroup.shader.vertex.destroy();
	}

	m_texturePool.cleanup();
}

// ------------------------------------------
// TRANSFORM
// ------------------------------------------

Transform::Transform() :
	position{ 1, 1, 1 }, scale{ 1, 1, 1 }, rotation{}
{}
Transform::Transform(Vector3 position, Vector3 scale, Quaternion rotation) :
	position{ position }, scale{ scale }, rotation{ rotation }
{}



// ------------------------------------------
// TINY OBJ LOADER HELPER
// ------------------------------------------

struct ObjMesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Material mat;
};
struct ObjData
{
	std::vector<ObjMesh> meshes;
};

template <>
struct std::hash<Vertex>
{
	std::size_t operator()(const Vertex& v) const
	{
		return    std::hash<float>()(v.pos.x)
				^ std::hash<float>()(v.pos.y) << 1
				^ std::hash<float>()(v.pos.z) >> 1
				^ std::hash<float>()(v.normal.x)
				^ std::hash<float>()(v.normal.y) << 2
				^ std::hash<float>()(v.normal.z) >> 2
				^ std::hash<float>()(v.uv.x) << 3
				^ std::hash<float>()(v.uv.y) >> 3
				^ std::hash<float>()(v.color.x) << 4
				^ std::hash<float>()(v.color.y) >> 4
				^ std::hash<float>()(v.color.z) << 5;
	}
};

void load_mesh(std::string file, ObjData& objData)
{
	file = ROOT_DIRECTORY + file;
	std::string directory;
	const size_t lastSlash = file.find_last_of("\\/");
	if (lastSlash != std::string::npos)
		directory = file.substr(0, lastSlash + 1);

	tinyobj::ObjReaderConfig readerConfig;
	readerConfig.mtl_search_path = "";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(file, readerConfig))
		if (!reader.Error().empty())
			log_err(reader.Error());

	if (!reader.Warning().empty())
		log(reader.Warning());

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// transferring data

	objData.meshes.clear();
	objData.meshes.reserve(shapes.size());

	std::set<Vertex> vertices;
	uint32_t i = 0;

	for (auto& shape : shapes)
	{
		objData.meshes.push_back(ObjMesh{});
		ObjMesh& mesh = objData.meshes.back();

		if (shape.mesh.material_ids.size() > 0)
		{
			auto matIndex = shape.mesh.material_ids[0];
			mesh.mat.m_texBaseDir = directory;
			mesh.mat = materials[matIndex];
		}

		int triIndex = 0;
		size_t indexIndex = 0;
		for (tinyobj::index_t index : shape.mesh.indices)
		{
			Vertex vert;
			vert.pos = Vector3 {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			if (index.normal_index >= 0)
			{
				vert.normal = Vector3{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}
			else
			{
				int offset = indexIndex % 3;
				int aIndex = (triIndex + 1) % 3;
				int bIndex = (triIndex + 2) % 3;
				tinyobj::index_t aInd = shape.mesh.indices[indexIndex - offset + aIndex];
				tinyobj::index_t bInd = shape.mesh.indices[indexIndex - offset + bIndex];
				Vector3 a = {
					attrib.vertices[3 * aInd.vertex_index + 0],
					attrib.vertices[3 * aInd.vertex_index + 1],
					attrib.vertices[3 * aInd.vertex_index + 2]
				};
				Vector3 b = {
					attrib.vertices[3 * bInd.vertex_index + 0],
					attrib.vertices[3 * bInd.vertex_index + 1],
					attrib.vertices[3 * bInd.vertex_index + 2]
				};
				vert.normal = glm::cross(
					a - vert.pos,
					b - vert.pos
				);
			}
			triIndex = (triIndex + 1) % 3;
			indexIndex++;
			if (index.texcoord_index >= 0)
			{
				vert.uv = Vector2{
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}
			vert.color = Vector3 {
				attrib.colors[3 * index.vertex_index + 0],
				attrib.colors[3 * index.vertex_index + 1],
				attrib.colors[3 * index.vertex_index + 2]
			};

			//vertices.insert(vert);

			mesh.vertices.push_back(vert);
			mesh.indices.push_back(i);
			i++;
			/*if (!vertices.contains(vert))
			{
				vertices[vert] = i;
				objData.indices.push_back(i);
				i++;
			}
			else
			{
				objData.indices.push_back(vertices.at(vert));
			}*/
		}
	}

	//objData.vertices.reserve(vertices.size());
	//for (const auto& kv : vertices)
	//	objData.vertices.push_back(kv.first);
}

// ------------------------------------------
// STATIC MODEL
// ------------------------------------------

void StaticModel::load_mesh(std::string file)
{
	ObjData objData;
	::load_mesh(file, objData);

	m_children.clear();
	m_children.reserve(objData.meshes.size());
	for (const auto& objMesh : objData.meshes)
	{
		m_children.push_back(StaticMesh{});
		StaticMesh& mesh = m_children.back();
		mesh.material = objMesh.mat;
		mesh.vertices = objMesh.vertices;
		mesh.indices = objMesh.indices;
	}
}