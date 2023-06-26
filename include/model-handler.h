#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "ecs.h"
#include "nve_types.h"
#include "material.h"

#include "buffer.h"

struct Submesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;
};

struct StaticMesh
{
	std::vector<Vertex> vertices;
	std::vector<Index> indices;

	Material material;
};

// 1,572,864 byte per segment (~1.5 MB)
#define VERTEX_GROUP_MAX_SIZE 32768

struct VertexGroup
{
	uint32_t start;
	uint32_t size;
	Material material;
};

struct MeshDataInfo
{
	uint32_t vertexStart;
	uint32_t vertexEnd;
	uint32_t indexStart;
	uint32_t indexEnd;

	Material material;
};

class StaticGeometryHandler : System<StaticMesh, Transform>
{
public:
	VkCommandBuffer secondary_command_buffer();

	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

private:
	void add_mesh(StaticMesh& mesh);
	bool new_mat(const Material& material);

	std::vector<Vertex> m_vertices;
	std::vector<Index> m_indices;
	std::vector<MeshDataInfo> m_meshes;

	Buffer<Vertex> m_vertexBuffer;
	Buffer<Index> m_indexBuffer;
};

class ModelHandler : System<Mesh, Transform>
{
public:

	void awake(EntityId entity, ECSManager& ecs) override;
	void update(float dt, ECSManager& ecs) override;

	void push_data(std::vector<VertexGroup> groups);

private:
	void add_mesh(Mesh& mesh);

	std::vector<VertexGroup> m_groups;
	bool new_mat(const Material& material);
	VertexGroup& get_group(const Material& material);
	void swap_groups(VertexGroup a, VertexGroup b);
	VertexGroup& new_group();
	void push_vertices(std::vector<Vertex>::const_iterator start, std::vector<Vertex>::const_iterator end);

	std::vector<Vertex> m_vertices;
	std::vector<Index> m_indices;
	std::vector<Vertex>::const_iterator fill_group(std::vector<Vertex>::const_iterator start, std::vector<Vertex>::const_iterator end, VertexGroup& group);
	void pad_vertices();
};