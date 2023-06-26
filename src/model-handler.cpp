#include "model-handler.h"

void StaticGeometryHandler::awake(EntityId entity, ECSManager& ecs)
{
	Transform& transform = ecs.get_component<Transform>(entity);
	StaticMesh& mesh = ecs.get_component<StaticMesh>(entity);

	add_mesh(mesh);
}

void ModelHandler::update(float dt, ECSManager& ecs)
{

}

void ModelHandler::push_data(std::vector<VertexGroup> groups)
{

}

void StaticGeometryHandler::add_mesh(StaticMesh& mesh)
{
	if (new_mat(mesh.material))
	{
		push_vertices(mesh.vertices.cbegin(), mesh.vertices.cend());
	}
	else
	{
		auto it = fill_group(mesh.vertices.cbegin(), mesh.vertices.cend(), get_group(mesh.material));
		push_vertices(it, mesh.vertices.cend());
	}
}
std::vector<Vertex>::const_iterator ModelHandler::fill_group(std::vector<Vertex>::const_iterator start, std::vector<Vertex>::const_iterator end, VertexGroup& group)
{
	auto it = m_vertices.begin() + group.start;
	auto vertIt = start;
	auto end = it + VERTEX_GROUP_MAX_SIZE - group.size;
	while (it != end && vertIt != end)
	{
		*it = *vertIt;
		it++;
		vertIt++;
	}
	group.size += vertIt - start;
	return vertIt;
}
void ModelHandler::pad_vertices()
{
	m_vertices.insert(m_vertices.end(), VERTEX_GROUP_MAX_SIZE - (m_vertices.size() % VERTEX_GROUP_MAX_SIZE), Vertex{  });
}
VertexGroup& ModelHandler::new_group()
{
	pad_vertices();
	m_groups.push_back(VertexGroup { (uint32_t) m_vertices.size() - VERTEX_GROUP_MAX_SIZE, 0, Material{ } });
	return m_groups.back();
}
void ModelHandler::push_vertices(std::vector<Vertex>::const_iterator start, std::vector<Vertex>::const_iterator end)
{
	auto it = start;
	while (it != end)
	{
		VertexGroup& group = new_group();
		it = fill_group(it, end, group);
	}
}
bool ModelHandler::new_mat(const Material& material)
{
	for (auto g : m_groups)
		if (g.material == material)
			return true;
	return false;
}
VertexGroup& ModelHandler::get_group(const Material& material)
{
	for (auto& g : m_groups)
		if (g.material == material)
			return g;
}
