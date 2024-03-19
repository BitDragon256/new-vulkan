#include "model-handler.h"

#include <set>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "math-core.h"
#include "profiler.h"

#define RENDER_PROFILER
#ifdef RENDER_PROFILER
#define PROFILE_START(X) m_profiler.start_measure(X);
#define PROFILE_END(X) m_profiler.end_measure(X, true);
#else
#define PROFILE_START(X)
#define PROFILE_END(X) 0.f;
#endif

// ------------------------------------------
// STATIC MESH
// ------------------------------------------

StaticMesh::StaticMesh() :
	id{ 0 }
{
	material = std::make_shared<Material>();
}

// ------------------------------------------
// NON-MEMBER FUNCTIONS
// ------------------------------------------

bool operator== (const MeshDataInfo& a, const MeshDataInfo& b)
{
	return
		a.indexStart == b.indexStart
		&& a.indexCount == b.indexCount
		&& a.vertexStart == b.vertexStart
		&& a.vertexCount == b.vertexCount
		&& a.meshGroup == b.meshGroup;
}
template<typename T>
void append_vector(std::vector<T>& origin, std::vector<T>& appendage)
{
	if (appendage.empty())
		return;
	origin.insert(origin.end(), appendage.begin(), appendage.end());
}
void bake_transform(StaticMesh& mesh, Transform transform)
{
	for (Vertex& vertex : mesh.vertices)
	{
		vertex.pos = Quaternion::rotate(vertex.pos * transform.scale, transform.rotation) + transform.position;
	}
}

void set_dynamic_state(VkCommandBuffer commandBuffer, VkExtent2D swapChainExtent, std::array<float, 4> viewportSize)
{
	VkViewport viewport = {};
	viewport.x = viewportSize[0];
	viewport.y = viewportSize[1];
	viewport.width = viewportSize[2];
	viewport.height = viewportSize[3];
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

// ------------------------------------------
// GEOMETRY HANDLER
// ------------------------------------------

//std::set<GraphicsShader_T*> GeometryHandler::s_destroyedShaders = std::set<GraphicsShader_T*>();
GeometryHandler::GeometryHandler() : m_subpassCount{ 0 }, m_pipelineLayout{}
{}

void GeometryHandler::initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* guiManager)
{
	m_vulkanObjects = vulkanObjects;
	reloadMeshBuffers = true;

	m_subpassCount = 0;
	m_rendererPipelinesCreated = false;

	BufferConfig matBufConf = default_buffer_config();
	matBufConf.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_materialBuffer.initialize(matBufConf);

	m_texturePool.init(m_vulkanObjects.device, m_vulkanObjects.physicalDevice, m_vulkanObjects.commandPool, m_vulkanObjects.transferQueue);

	create_descriptor_set();
	create_pipeline_layout();

	m_guiManager = guiManager;
}
void GeometryHandler::update_framebuffers(std::vector<VkFramebuffer> framebuffers, VkExtent2D swapchainExtent)
{
	m_vulkanObjects.framebuffers = framebuffers;
	m_vulkanObjects.swapchainExtent = swapchainExtent;
}
void GeometryHandler::set_first_subpass(uint32_t subpass)
{
	m_vulkanObjects.firstSubpass = subpass;
}
// TODO staged buffer copy synchronization
std::vector<VkSemaphore> GeometryHandler::buffer_cpy_semaphores()
{
	// ANCHOR
	vkQueueWaitIdle(m_vulkanObjects.transferQueue);
	vkDeviceWaitIdle(m_vulkanObjects.device);
	return {};

	std::vector<VkSemaphore> sems = {};
	sems.push_back(m_materialBuffer.staged_buf_cpy_semaphore());
	for (auto& meshGroup : m_meshGroups)
	{
		sems.push_back(meshGroup.vertexBuffer.staged_buf_cpy_semaphore());
		sems.push_back(meshGroup.indexBuffer.staged_buf_cpy_semaphore());
	}

	return sems;
}
std::vector<VkFence> GeometryHandler::buffer_cpy_fences()
{
	std::vector<VkFence> fences = {};
	fences.push_back(m_materialBuffer.staged_buf_cpy_fence());
	for (auto& meshGroup : m_meshGroups)
	{
		fences.push_back(meshGroup.vertexBuffer.staged_buf_cpy_fence());
		fences.push_back(meshGroup.indexBuffer.staged_buf_cpy_fence());
	}
	return fences;
}

MeshGroup* GeometryHandler::find_group(GraphicsShader& shader, size_t& index)
{
	if (!shader)
	{
		// automatically assign the first best
		if (m_meshGroups.size() > 0)
		{
			shader = m_meshGroups[0].shader.lock();
			return &m_meshGroups[0];
		}
		return nullptr;
	}

	index = 0;
	while (index < m_meshGroups.size())
	{
		if ((*m_meshGroups[index].shader.lock()) == (*shader))
			return &m_meshGroups[index];
		index++;
	}
	return nullptr;
}
MeshGroup* GeometryHandler::create_mesh_group(GraphicsShader& shader)
{
	m_meshGroups.push_back(MeshGroup {});
	auto& meshGroup = m_meshGroups.back();
	if (!shader)
	{
		meshGroup.shader = make_default_shader();
		shader = meshGroup.shader.lock();
	}
	else
	{
		meshGroup.shader = shader;
	}

	BufferConfig config = default_buffer_config();
	config.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	meshGroup.vertexBuffer.initialize(config);

	config.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	meshGroup.indexBuffer.initialize(config);

	create_group_command_buffers(meshGroup);
      m_subpassCount++;

	return &meshGroup;
}

//void GeometryHandler::create_command_buffers()
//{
//	for (MeshGroup& meshGroup : m_meshGroups)
//		for (VkCommandBuffer& commandBuffer : meshGroup.commandBuffers)
//			create_command_buffer(&commandBuffer);
//}
void GeometryHandler::create_group_command_buffers(MeshGroup& meshGroup)
{
	meshGroup.commandBuffers.resize(m_vulkanObjects.framebuffers.size());

	VkCommandPoolCreateInfo commandPoolCI = {};
	commandPoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCI.pNext = nullptr;
	commandPoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCI.queueFamilyIndex = m_vulkanObjects.queueFamilyIndex;
	vkCreateCommandPool(m_vulkanObjects.device, &commandPoolCI, nullptr, &meshGroup.commandPool);

	VkCommandBufferAllocateInfo commandBufferAI = {};
	commandBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAI.pNext = nullptr;
	commandBufferAI.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	commandBufferAI.commandPool = meshGroup.commandPool;
	commandBufferAI.commandBufferCount = static_cast<uint32_t>(m_vulkanObjects.framebuffers.size());
	auto res = vkAllocateCommandBuffers(m_vulkanObjects.device, &commandBufferAI, meshGroup.commandBuffers.data());
	logger::log_cond_err(res == VK_SUCCESS, "failed to allocate command buffer for the static geometry handler");
}
void GeometryHandler::record_command_buffers(uint32_t frame)
{
	// record command buffers
	uint32_t subpass = m_vulkanObjects.firstSubpass;
	for (size_t i = 0; i < m_meshGroups.size(); i++)
	{
		record_command_buffer(subpass, frame, m_meshGroups[i], i);
		subpass++;
	}
}
std::vector<VkCommandBuffer> GeometryHandler::get_command_buffers(uint32_t frame)
{
	std::vector<VkCommandBuffer> buffers;

	for (MeshGroup& meshGroup : m_meshGroups)
		buffers.push_back(meshGroup.commandBuffers[static_cast<size_t>(frame)]);

	return buffers;
}

void GeometryHandler::create_pipeline_create_infos()
{
	uint32_t subpass = m_vulkanObjects.firstSubpass;
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		meshGroup.pipeline.initialize(m_vulkanObjects.device, &m_pipelineLayout, m_vulkanObjects.renderPass, subpass, meshGroup.shader);
		meshGroup.pipeline.create_create_info();

		subpass++;
	}

	m_subpassCount = m_meshGroups.size();
	m_rendererPipelinesCreated = true;
}
void GeometryHandler::get_pipelines(std::vector<PipelineRef>& pipelines)
{
	create_pipeline_create_infos();
	for (size_t i = 0; i < m_meshGroups.size(); i++)
	{
		pipelines.push_back(&m_meshGroups[i].pipeline);
	}
}

void GeometryHandler::create_pipeline_layout()
{
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { m_descriptorSetLayout };

	VkPushConstantRange cameraPushConstantRange = {};
	cameraPushConstantRange.offset = 0;
	cameraPushConstantRange.size = sizeof(CameraPushConstant);
	cameraPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkPushConstantRange> pushConstants = { cameraPushConstantRange };

	m_pipelineLayout.create(descriptorSetLayouts, pushConstants, m_vulkanObjects.device);

}

void GeometryHandler::add_material(Model& model, Transform& transform, bool newMat)
{
	if (!newMat)
	{
		// material of first child already has to be in m_materials
		uint32_t i = 0;
		for (auto mat : m_materials)
		{
			if (*mat == (const Material&) model.m_children[0].material)
			{
				transform.materialStart = i;
				return;
			}
			i++;
		}
	}
	
	transform.materialStart = m_materials.size();
	uint32_t i = 0;
	for (auto& mesh : model.m_children)
	{
		m_materials.push_back(mesh.material);
		for (auto& v : mesh.vertices)
			v.material = i;
		i++;
	}
}
void GeometryHandler::add_model(Model& model, bool forceNewMeshGroup)
{
	for (auto& mesh : model.m_children)
	{
		size_t index;
		MeshGroup* meshGroup = find_group(mesh.material->m_shader, index);
		if (!meshGroup || forceNewMeshGroup) // no group found or a new group must be created
			meshGroup = create_mesh_group(mesh.material->m_shader);

		//if (s_destroyedShaders.contains(meshGroup->shader))
		//	s_destroyedShaders.erase(meshGroup->shader);

		// save mesh
		size_t meshId = meshGroup->meshes.size();
		mesh.id = meshId;
		meshGroup->meshes.push_back(MeshDataInfo{
			meshGroup->vertices.size(),
			mesh.vertices.size(),
			meshGroup->indices.size(),
			mesh.indices.size(),
			index,
			meshId
		});
		append_vector(meshGroup->vertices, mesh.vertices);
		//for (Index& index : mesh.indices)
		//	index += meshGroup->indices.size();
		append_vector(meshGroup->indices, mesh.indices);

		meshGroup->reloadMeshBuffers = true;
		reloadMeshBuffers = true;
	}
}
void GeometryHandler::remove_model(Model& model)
{
	for (auto& mesh : model.m_children)
	{
		size_t index;
		MeshGroup* meshGroup = find_group(mesh.material->m_shader, index);
		if (!meshGroup)
			continue;

		MeshDataInfo meshInfo;
		bool found = false;
		for (const auto& mi : meshGroup->meshes)
		{
                  if (mi.meshId == mesh.id)
                  {
                        meshInfo = mi;
				found = true;
				break;
                  }
		}
		if (!found)
			continue;
		//for (auto& mi : meshGroup->meshes)
		//{
		//	if (mi.vertexStart > meshInfo.vertexStart)
  //                      mi.vertexStart -= meshInfo.vertexCount;
		//	if (mi.indexStart > meshInfo.indexStart)
  //                      mi.indexStart -= meshInfo.indexCount;
		//}

		//meshGroup->vertices.erase(
		//	meshGroup->vertices.begin() + meshInfo.vertexStart,
		//	meshGroup->vertices.begin() + meshInfo.vertexStart + meshInfo.vertexCount
		//);
		//meshGroup->indices.erase(
		//	meshGroup->indices.begin() + meshInfo.indexStart,
		//	meshGroup->indices.begin() + meshInfo.indexStart + meshInfo.indexCount
		//);
		std::erase(meshGroup->meshes, meshInfo);
		std::erase(m_materials, mesh.material);

		meshGroup->reloadMeshBuffers = true;
		reloadMeshBuffers = true;
	}
}
void GeometryHandler::reload_materials()
{
	m_profiler.begin_label("reload materials");

	PROFILE_START("iterate through mats");
	std::vector<MaterialSSBO> mats(m_materials.size());
	for (size_t i = 0; i < mats.size(); i++)
	{
            mats[i] = *m_materials[i];
            if (!m_materials[i]->m_diffuseTex.empty())
                  mats[i].m_textureIndex = m_texturePool.find((*m_materials[i]).m_diffuseTex);
            else
                  mats[i].m_textureIndex = UINT32_MAX;
	}
	PROFILE_END("iterate through mats");
	PROFILE_START("update buffer");
	bool reload = m_materialBuffer.set(mats);
	PROFILE_END("update buffer");
	if (reload)
	{
		PROFILE_START("update descriptor");
		update_descriptor_set();
		PROFILE_END("update descriptor");
	}

	m_profiler.end_label();
}
void GeometryHandler::reload_meshes()
{
	reload_materials();

	// reload meshes
	for (MeshGroup& meshGroup : m_meshGroups)
		if (meshGroup.reloadMeshBuffers)
			reload_mesh_group(meshGroup);

	reloadMeshBuffers = false;
}
void GeometryHandler::reload_mesh_group(MeshGroup& meshGroup)
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	for (const auto& meshInfo : meshGroup.meshes)
	{
		Index vertexIndexDelta = static_cast<Index>(vertices.size());
		vertices.insert(
			vertices.end(),
			meshGroup.vertices.cbegin() + meshInfo.vertexStart,
			meshGroup.vertices.cbegin() + meshInfo.vertexStart + meshInfo.vertexCount
		);
		size_t indexStart = indices.size();
		indices.insert(
			indices.end(),
			meshGroup.indices.cbegin() + meshInfo.indexStart,
			meshGroup.indices.cbegin() + meshInfo.indexStart + meshInfo.indexCount
		);
		for (auto it = indices.begin() + indexStart; it != indices.end(); it++)
		{
			*it += vertexIndexDelta;
		}
	}

	meshGroup.vertexBuffer.set(vertices);
	meshGroup.indexBuffer.set(indices);

	meshGroup.reloadMeshBuffers = false;
}
BufferConfig GeometryHandler::default_buffer_config()
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

uint32_t GeometryHandler::subpass_count()
{
	return m_subpassCount;
}

VkWriteDescriptorSet GeometryHandler::material_buffer_descriptor_set_write()
{
	m_materialBufferDescriptorInfo = {};
	m_materialBufferDescriptorInfo.buffer = m_materialBuffer.m_buffer;
	m_materialBufferDescriptorInfo.offset = 0;
	m_materialBufferDescriptorInfo.range = m_materialBuffer.range();

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_descriptorSet;
	write.dstBinding = GEOMETRY_HANDLER_MATERIAL_BINDING;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.pBufferInfo = &m_materialBufferDescriptorInfo;

	return write;
}

void GeometryHandler::create_descriptor_set()
{
	VkDescriptorSetLayoutBinding materialBufferBinding = {};
	materialBufferBinding.descriptorCount = 1;
	materialBufferBinding.binding = GEOMETRY_HANDLER_MATERIAL_BINDING;
	materialBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	materialBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	materialBufferBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding texturePoolBinding = {};
	texturePoolBinding.descriptorCount = TEXTURE_POOL_MAX_TEXTURES;
	texturePoolBinding.binding = GEOMETRY_HANDLER_TEXTURE_BINDING;
	texturePoolBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	texturePoolBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	texturePoolBinding.pImmutableSamplers = nullptr;
	
	VkDescriptorSetLayoutBinding samplerBinding = {};
	samplerBinding.descriptorCount = 1;
	samplerBinding.binding = GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING;
	samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	samplerBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
		materialBufferBinding,
		texturePoolBinding,
		samplerBinding,
	};
	auto otherDescriptors = other_descriptors();
	append_vector(layoutBindings, otherDescriptors);

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = {};
	descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCI.pNext = nullptr;
	descriptorSetLayoutCI.flags = 0;
	descriptorSetLayoutCI.bindingCount = layoutBindings.size();
	descriptorSetLayoutCI.pBindings = layoutBindings.data();

	{
		auto res = vkCreateDescriptorSetLayout(m_vulkanObjects.device, &descriptorSetLayoutCI, nullptr, &m_descriptorSetLayout);
		logger::log_cond_err(res == VK_SUCCESS, "failed to create static geometry handler descriptor set layout");
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
		logger::log_cond_err(res == VK_SUCCESS, "failed to allocate static geometry handler descriptor set");
	}
}
void GeometryHandler::update_descriptor_set()
{
	std::vector<VkWriteDescriptorSet> writes;

	writes.push_back(material_buffer_descriptor_set_write());
	writes.push_back(m_texturePool.get_descriptor_set_write(m_descriptorSet, GEOMETRY_HANDLER_TEXTURE_BINDING));
	if (!writes.back().pImageInfo)
		writes.pop_back();

	writes.push_back(m_texturePool.get_sampler_descriptor_set_write(m_descriptorSet, GEOMETRY_HANDLER_TEXTURE_SAMPLER_BINDING));

	vkUpdateDescriptorSets(
		m_vulkanObjects.device,
		static_cast<uint32_t>(writes.size()),
		writes.data(),
		0, nullptr
	);
}

void GeometryHandler::cleanup()
{
	for (MeshGroup& meshGroup : m_meshGroups)
	{
		meshGroup.indexBuffer.destroy();
		meshGroup.vertexBuffer.destroy();

		meshGroup.pipeline.destroy();

		vkFreeCommandBuffers(m_vulkanObjects.device, meshGroup.commandPool, meshGroup.commandBuffers.size(), meshGroup.commandBuffers.data());
		vkDestroyCommandPool(m_vulkanObjects.device, meshGroup.commandPool, nullptr);

		auto shaderPtr = meshGroup.shader.lock();
		//if (shaderPtr && !s_destroyedShaders.contains(meshGroup.shader))
		if (shaderPtr)
		{
			shaderPtr->fragment.destroy();
			shaderPtr->vertex.destroy();
			//s_destroyedShaders.insert(meshGroup.shader);
		}
	}

	m_materialBuffer.destroy();

	m_pipelineLayout.destroy();
	vkFreeDescriptorSets(m_vulkanObjects.device, m_vulkanObjects.descriptorPool, 1, &m_descriptorSet);
	vkDestroyDescriptorSetLayout(m_vulkanObjects.device, m_descriptorSetLayout, nullptr);

	m_texturePool.cleanup();
}

void GeometryHandler::update()
{
	if (reloadMeshBuffers)
	{
		PROFILE_START("reload meshes");
		reload_meshes();
		reloadMeshBuffers = false;
		PROFILE_END("reload meshes");
	}
	else
	{
		PROFILE_START("reload mats");
		reload_materials();
		PROFILE_END("reload mats");
	}
}

// ------------------------------------------
// STATIC GEOMETRY HANDLER
// ------------------------------------------

StaticGeometryHandler::StaticGeometryHandler()
{
	m_subpassCount = 1;
}
void StaticGeometryHandler::load_dummy_model()
{
	StaticMesh mesh;
	mesh.vertices = { NULL_VERTEX, NULL_VERTEX, NULL_VERTEX };
	mesh.indices = { 0, 1, 2 };
	mesh.material->m_shader = make_default_shader();
	mesh.material->m_shader->fragment.load_shader("fragments/unlit_wmat.frag.spv");
	mesh.material->m_shader->vertex.load_shader("vertex/static_wmat.vert.spv");

	m_dummyModel.m_children.push_back(mesh);
	Transform transform;
	add_model(m_dummyModel, transform);
}
void StaticGeometryHandler::initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* gui)
{
	GeometryHandler::initialize(vulkanObjects, gui);
	load_dummy_model();
}
void StaticGeometryHandler::add_model(StaticModel& model, Transform& transform)
{
	for (auto& mesh : model.m_children)
	{
		bake_transform(mesh, transform);
	}

	GeometryHandler::add_material(model, transform, GEOMETRY_HANDLER_INDEPENDENT_MATERIALS);
	GeometryHandler::add_model(model);
}
void StaticGeometryHandler::record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex)
{
	VkCommandBuffer commandBuffer = meshGroup.commandBuffers[frame];
	vkResetCommandBuffer(commandBuffer, 0);

	// ---------------------------------------

	VkCommandBufferInheritanceInfo inheritanceInfo;
	VkCommandBufferBeginInfo commandBufferBI = create_command_buffer_begin_info(*m_vulkanObjects.renderPass, subpass, m_vulkanObjects.framebuffers[static_cast<size_t>(frame)], inheritanceInfo);

	{
		auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
		logger::log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording for the static geometry handler");
	}

	// ---------------------------------------

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshGroup.pipeline);

	// ---------------------------------------

	set_dynamic_state(
		commandBuffer,
		m_vulkanObjects.swapchainExtent,
		m_guiManager->viewport({
			0, 0,
			static_cast<float>(m_vulkanObjects.swapchainExtent.width),
			static_cast<float>(m_vulkanObjects.swapchainExtent.height)
		}));

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

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshGroup.indexBuffer.size()), 1, 0, 0, 0);

	// ---------------------------------------

	{
		auto res = vkEndCommandBuffer(commandBuffer);
		logger::log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording for the static geometry handler");
	}

	// ---------------------------------------
}
std::vector<VkDescriptorSetLayoutBinding> StaticGeometryHandler::other_descriptors()
{
	return {};
}

void StaticGeometryHandler::awake(EntityId entity)
{
	auto transform = m_ecs->get_component<Transform>(entity);
	auto& model = m_ecs->get_component<StaticModel>(entity);
	add_model(model, transform);
}
void StaticGeometryHandler::update(float dt)
{
	GeometryHandler::update();
}

// ------------------------------------------
// DYNAMIC MODEL HANDLER
// ------------------------------------------

DynamicGeometryHandler::DynamicGeometryHandler()
{
	m_subpassCount = 0;
}
void DynamicGeometryHandler::start()
{
	BufferConfig bufferConfig = default_buffer_config();
	bufferConfig.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_transformBuffer.initialize(bufferConfig);
	m_updatedTransformDescriptorSets = false;

	m_modelCount = 0;
}
void DynamicGeometryHandler::awake(EntityId entity)
{
	auto& transform = m_ecs->get_component<Transform>(entity);
	auto& model = m_ecs->get_component<DynamicModel>(entity);
	add_model(model, transform);
}
void DynamicGeometryHandler::update(float dt)
{
	m_profiler.begin_label("dyn update");
	PROFILE_START("get transforms")
	// get all transforms
	std::vector<Transform> transforms(m_entities.size());
	for (size_t i = 0; i < m_entities.size(); i++)
		transforms[i] = m_ecs->get_component<Transform>(m_entities[i]);
	PROFILE_END("get transforms");

	PROFILE_START("push transforms");
	// push transforms
	bool updateDescriptorSet = m_transformBuffer.set(transforms);
	PROFILE_END("push transforms");

	PROFILE_START("update base handler");
	GeometryHandler::update();
	PROFILE_END("update base handler");

	// update descriptor set
	if (updateDescriptorSet || !m_updatedTransformDescriptorSets)
	{
		VkWriteDescriptorSet transformBufferWrite = {};
		transformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		transformBufferWrite.pNext = nullptr;

		VkDescriptorBufferInfo transformBufferInfo = {};
		transformBufferInfo.buffer = m_transformBuffer.m_buffer;
		transformBufferInfo.offset = 0;
		transformBufferInfo.range = m_transformBuffer.range();

		transformBufferWrite.descriptorCount = 1;
		transformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		transformBufferWrite.dstBinding = DYNAMIC_MODEL_HANDLER_TRANSFORM_BUFFER_BINDING;
		transformBufferWrite.dstSet = m_descriptorSet;
		transformBufferWrite.pBufferInfo = &transformBufferInfo;

		PROFILE_START("update descriptors");
		vkUpdateDescriptorSets(m_vulkanObjects.device, 1, &transformBufferWrite, 0, nullptr);
		PROFILE_END("update descriptors");

		m_updatedTransformDescriptorSets = true;
	}
	m_profiler.end_label();
}
// TODO staged buffer copy synchronization
std::vector<VkSemaphore> DynamicGeometryHandler::buffer_cpy_semaphores()
{
	auto sems = GeometryHandler::buffer_cpy_semaphores();
	sems.push_back(m_transformBuffer.staged_buf_cpy_semaphore());

	return sems;
}
// TODO staged buffer copy synchronization
std::vector<VkFence> DynamicGeometryHandler::buffer_cpy_fences()
{
	auto fences = GeometryHandler::buffer_cpy_fences();
	fences.push_back(m_transformBuffer.staged_buf_cpy_fence());

	return fences;
}

void DynamicGeometryHandler::cleanup()
{
	m_transformBuffer.destroy();

	GeometryHandler::cleanup();
}

void DynamicGeometryHandler::record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex)
{
	VkCommandBuffer commandBuffer = meshGroup.commandBuffers[frame];
	vkResetCommandBuffer(commandBuffer, 0);

	// ---------------------------------------

	VkCommandBufferInheritanceInfo inheritanceInfo;
	VkCommandBufferBeginInfo commandBufferBI = create_command_buffer_begin_info(*m_vulkanObjects.renderPass, subpass, m_vulkanObjects.framebuffers[static_cast<size_t>(frame)], inheritanceInfo);

	{
		auto res = vkBeginCommandBuffer(commandBuffer, &commandBufferBI);
		logger::log_cond_err(res == VK_SUCCESS, "failed to begin command buffer recording for the static geometry handler");
	}

	// ---------------------------------------

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshGroup.pipeline);

	// ---------------------------------------

	set_dynamic_state(
		commandBuffer,
		m_vulkanObjects.swapchainExtent,
		m_guiManager->viewport({
			0, 0,
			static_cast<float>(m_vulkanObjects.swapchainExtent.width),
			static_cast<float>(m_vulkanObjects.swapchainExtent.height)
		}));

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

	auto modelInfo = m_individualModels[meshGroupIndex];
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshGroup.indexBuffer.size()), modelInfo.instanceCount, 0, 0, modelInfo.startIndex);

	// ---------------------------------------

	{
		auto res = vkEndCommandBuffer(commandBuffer);
		logger::log_cond_err(res == VK_SUCCESS, "failed to end command buffer recording for the static geometry handler");
	}

	// ---------------------------------------
}
void DynamicGeometryHandler::add_model(DynamicModel& model, Transform& transform)
{
	auto hashSum = hash_model(model);

	bool newMeshGroup = true;
	for (auto& modelInfo : m_individualModels)
	{
		if (hashSum == modelInfo.hashSum)
		{
			modelInfo.instanceCount++;
			newMeshGroup = false;
			break;
		}
	}
	GeometryHandler::add_material(model, transform, GEOMETRY_HANDLER_INDEPENDENT_MATERIALS);
	if (newMeshGroup)
	{
		GeometryHandler::add_model(model, true);
		DynamicModelInfo newModelInfo = {};
		newModelInfo.hashSum = hashSum;
		newModelInfo.instanceCount = 1;
		newModelInfo.startIndex = m_modelCount;
		m_individualModels.push_back(newModelInfo);
	}

	m_modelCount++;
}
std::vector<VkDescriptorSetLayoutBinding> DynamicGeometryHandler::other_descriptors()
{
	VkDescriptorSetLayoutBinding transformBufferBinding = {};
	transformBufferBinding.descriptorCount = 1;
	transformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	transformBufferBinding.pImmutableSamplers = nullptr;
	transformBufferBinding.binding = DYNAMIC_MODEL_HANDLER_TRANSFORM_BUFFER_BINDING;
	transformBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	return {
		transformBufferBinding,
	};
}

DynamicModelHashSum hash_model(const DynamicModel& model)
{
	DynamicModelHashSum hashSum = 263357457;
	for (auto child : model.m_children)
	{
		for (auto index : child.indices)
		{
			if (index >= child.vertices.size())
				continue;
			hashSum ^= (DynamicModelHashSum) (child.vertices[index].pos.x * 23626325 + child.vertices[index].pos.y * 9738346 + child.vertices[index].pos.z * 283756898967) * index + 2355901;
			hashSum >>= 11;
		}
		hashSum ^= (DynamicModelHashSum) child.material->m_shader.get();
	}
	return hashSum;
}

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

Profiler meshProfiler;

void load_mesh(std::string file, ObjData& objData)
{
	meshProfiler.start_measure("total loading time");
	meshProfiler.start_measure("tiny obj loader loading time");

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
			logger::log_err(reader.Error());

	if (!reader.Warning().empty())
		logger::log(reader.Warning());

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	meshProfiler.end_measure("tiny obj loader loading time");

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
			if (matIndex >= 0)
			{
				mesh.mat.m_texBaseDir = directory;
				mesh.mat = materials[matIndex];
			}
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

	meshProfiler.end_measure("total loading time");

	meshProfiler.print_last_measure("tiny obj loader loading time");
	meshProfiler.print_last_measure("total loading time");

	//objData.vertices.reserve(vertices.size());
	//for (const auto& kv : vertices)
	//	objData.vertices.push_back(kv.first);
}

// ------------------------------------------
// MODEL
// ------------------------------------------

void Model::load_mesh(std::string file)
{
	ObjData objData;
	::load_mesh(file, objData);

	m_children.clear();
	m_children.reserve(objData.meshes.size());

	meshProfiler.passing_measure("model transfer");
	for (const auto& objMesh : objData.meshes)
	{
		m_children.push_back(StaticMesh{});
		StaticMesh& mesh = m_children.back();
		mesh.material = std::make_shared<Material>();
		*mesh.material = objMesh.mat;
		mesh.vertices = objMesh.vertices;
		mesh.indices = objMesh.indices;
		mesh.material->m_shader = make_default_shader();
		mesh.material->m_diffuse = Color(1.f);
		meshProfiler.passing_measure("model transfer");
		meshProfiler.print_last_measure("model transfer");
	}
}
void Model::set_fragment_shader(std::string file)
{
	for (auto& mesh : m_children)
		mesh.material->m_shader->fragment.load_shader(file);
}
void Model::set_vertex_shader(std::string file)
{
	for (auto& mesh : m_children)
		mesh.material->m_shader->vertex.load_shader(file);
}