#include "gizmos.h"

// -----------------------------------------------------------------
// GIZMOS HANDLER
// -----------------------------------------------------------------

void GizmosHandler::draw_line(Vector3 start, Vector3 end, Color color, float width)
{
	EntityId e = m_ecs->create_entity();
	auto& model = m_ecs->add_component<GizmosModel>(e);
	model.load_mesh("/default_models/gizmos/cylinder.obj");
	auto& transform = m_ecs->add_component<Transform>(e);
	transform.position = (start + end) / 2.f;
	transform.rotation = Quaternion(end - start, VECTOR_RIGHT);
	transform.scale.x = glm::length(start - end);
	transform.scale.y = width;
	transform.scale.z = width;
}

void GizmosHandler::draw_ray(Vector3 start, Vector3 direction, Color color, float width)
{
	draw_line(start, start + direction, color, width);
}

void GizmosHandler::awake(EntityId entity)
{
	auto& transform = m_ecs->get_component<Transform>(entity);
	auto& model = m_ecs->get_component<GizmosModel>(entity);
	add_model(model, transform);
	m_newEntities.push_back(entity);
}
void GizmosHandler::update(float dt)
{
	GeometryHandler::update();
	//std::vector<EntityId> lastEntities;
	//std::set_difference(
	//	m_entities.cbegin(),
	//	m_entities.cend(),
	//	m_newEntities.cbegin(),
	//	m_newEntities.cend(),
	//	std::back_inserter(lastEntities)
	//);
	//m_newEntities.clear();
	////for (auto e : lastEntities)
	for (auto e : m_entities)
	{
		m_ecs->delete_entity(e);
	}
}
void GizmosHandler::remove(EntityId entity)
{
	GeometryHandler::remove_model(m_ecs->get_component<GizmosModel>(entity));
}

// -----------------------------------------------------------------
// GEOMETRY HANDLER STUFF
// -----------------------------------------------------------------

GizmosHandler::GizmosHandler()
{
}
void GizmosHandler::initialize(GeometryHandlerVulkanObjects vulkanObjects, GUIManager* gui)
{
	GeometryHandler::initialize(vulkanObjects, gui);
}
void GizmosHandler::add_model(GizmosModel& model, Transform& transform)
{
	for (auto& mesh : model.m_children)
	{
            mesh.material->m_shader = new GraphicsShader();
		mesh.material->m_shader->fragment.load_shader("fragments/unlit_wmat.frag.spv");
		mesh.material->m_shader->vertex.load_shader("vertex/static_wmat.vert.spv");
		bake_transform(mesh, transform);
	}

	GeometryHandler::add_material(model, transform, GEOMETRY_HANDLER_INDEPENDENT_MATERIALS);
	GeometryHandler::add_model(model);
}
void GizmosHandler::record_command_buffer(uint32_t subpass, size_t frame, const MeshGroup& meshGroup, size_t meshGroupIndex)
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
std::vector<VkDescriptorSetLayoutBinding> GizmosHandler::other_descriptors()
{
	return {};
}

