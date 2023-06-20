#include "models.h"

#include <numeric>
#include <functional>

// ----------------------------------
// ModelHandler
// ----------------------------------

ModelHandler::ModelHandler()
{
	reset();
}
void ModelHandler::init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool cmdPool, VkQueue transferQueue)
{
	reset();

	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = cmdPool;
	m_transferQueue = transferQueue;

	init_buffers();

	m_initialized = true;
}
void ModelHandler::destroy_buffers()
{
	m_vertexBuffer.destroy();
	m_indexBuffer.destroy();
	m_modelBuffer.destroy();
}
void ModelHandler::reset()
{
	m_vertexCount = 0;
	m_indexCount = 0;

	m_initialized = false;
	m_dataChanged = true;
	m_buffersCreated = false;

	m_models.clear();
}
void ModelHandler::upload_model_info()
{
	std::vector<Transform> modelData(m_models.size());
	auto modelDataLast = modelData.begin();

	for (Model* model : m_models)
	{
		*modelDataLast = model->m_info;
		modelDataLast++;
	}

	m_modelBuffer.set(modelData);
}
void ModelHandler::upload_mesh_data()
{
	if (!m_initialized)
	{
		log("model handler must be initialized before uploading data");
		return;
	}
	if (!m_dataChanged)
		return;

	std::vector<Vertex> vertexData(m_vertexCount);
	std::vector<Index> indexData(m_indexCount);

	auto vertexDataLast = vertexData.begin();
	auto indexDataLast = indexData.begin();

	for (Model* model : m_models)
	{
		model->write_vertices_to(vertexDataLast);
		model->write_indices_to(indexDataLast);
	}

	m_vertexBuffer.set(vertexData);
	m_indexBuffer.set(indexData);

	m_dataChanged = false;
}
void ModelHandler::init_buffers()
{
	BufferConfig config = {};
	config.device = m_device;
	config.physicalDevice = m_physicalDevice;
	config.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;//
	config.useStagedBuffer = true;
	config.stagedBufferTransferCommandPool = m_commandPool;
	config.stagedBufferTransferQueue = m_transferQueue;

	config.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	m_vertexBuffer.initialize(config);

	config.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	m_indexBuffer.initialize(config);

	config.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_modelBuffer.initialize(config);
}
void ModelHandler::create_buffers()
{
	m_vertexBuffer.recreate();
	m_indexBuffer.recreate();
	m_modelBuffer.recreate();

	m_buffersCreated = true;
}

void ModelHandler::add_model(Model* pModel)
{
	m_models.push_back(pModel);

	m_vertexCount += pModel->vertex_count();
	m_indexCount += pModel->index_count();

	m_dataChanged = true;
}

VkBuffer ModelHandler::vertex_buffer()
{
	assert_buffer_creation();
	return m_vertexBuffer.m_buffer;
}
VkBuffer ModelHandler::index_buffer()
{
	assert_buffer_creation();
	return m_indexBuffer.m_buffer;
}
VkBuffer ModelHandler::model_buffer()
{
	assert_buffer_creation();
	return m_modelBuffer.m_buffer;
}
uint32_t ModelHandler::vertex_count()
{
	return static_cast<uint32_t>(m_vertexCount);
}
uint32_t ModelHandler::index_count()
{
	return static_cast<uint32_t>(m_indexCount);
}

void ModelHandler::assert_buffer_creation()
{
	if (!m_buffersCreated)
		create_buffers();
}

// ----------------------------------
// Model
// ----------------------------------

size_t Model::vertex_count()
{
	return std::accumulate(m_meshes.begin(), m_meshes.end(), (size_t) 0, [](size_t acc, Mesh mesh) { return acc + mesh.vertex_count(); });
}
size_t Model::index_count()
{
	return std::accumulate(m_meshes.begin(), m_meshes.end(), (size_t) 0, [](size_t acc, Mesh mesh) { return acc + mesh.index_count(); });
}

void Model::write_vertices_to(std::vector<Vertex>::iterator dst)
{
	for (Mesh mesh : m_meshes)
	{
		mesh.write_vertices_to(dst);
	}
}
void Model::write_indices_to(std::vector<Index>::iterator dst)
{
	for (Mesh mesh : m_meshes)
	{
		mesh.write_indices_to(dst);
	}
}

Model Model::create_model(Mesh mesh)
{
	Model model;
	model.m_info = Transform { Vector3(0, 0, 0), Vector3(1, 1, 1), Quaternion(1.0, 0.0, 0.0, 0.0) };
	model.m_meshes = std::vector<Mesh>(1);
	model.m_meshes[0] = mesh;
	return model;
}

// ----------------------------------
// Mesh
// ----------------------------------

size_t Mesh::vertex_count()
{
	return std::accumulate(m_submeshes.begin(), m_submeshes.end(), (size_t) 0, [](size_t acc, Submesh mesh) { return acc + mesh.vertices.size(); });
}
size_t Mesh::index_count()
{
	return std::accumulate(m_submeshes.begin(), m_submeshes.end(), (size_t) 0, [](size_t acc, Submesh mesh) { return acc + mesh.indices.size(); });
}

void Mesh::write_vertices_to(std::vector<Vertex>::iterator dst)
{
	for (Submesh submesh : m_submeshes)
	{
		std::copy(submesh.vertices.begin(), submesh.vertices.end(), dst);
	}
}
void Mesh::write_indices_to(std::vector<Index>::iterator dst)
{
	for (Submesh submesh : m_submeshes)
	{
		std::copy(submesh.indices.begin(), submesh.indices.end(), dst);
	}
}

Mesh Mesh::create_simple_mesh(std::vector<Vertex> vertices, std::vector<Index> indices)
{
	Mesh mesh;
	mesh.m_submeshes = std::vector<Submesh>(1);
	mesh.m_submeshes[0].vertices = vertices;
	mesh.m_submeshes[0].indices = indices;
	return mesh;
}
Mesh Mesh::create_cube()
{
	//   B _______ C
	//    /      /|
	// A /______/ | D
	//  F||_____|_|G
	//   |/     |/
	// E |______| H
	//
	//    ^ x3
	//    |
	//    |_____> x2
	//   /
	//  /
	// L x1
	//
	// clockwise triangle
	//
	//       /\ 1.)
	//      /  \
	// 3.) /____\ 2.)

	return create_simple_mesh(
		{
			{ Vector3( 0.5, -0.5,  0.5), Vector3(1.0, 0.0, 0.0), Vector2(0.0, 0.0) }, // A 0
			{ Vector3(-0.5, -0.5,  0.5), Vector3(0.0, 1.0, 0.0), Vector2(0.0, 0.0) }, // B 1
			{ Vector3(-0.5,  0.5,  0.5), Vector3(0.0, 0.0, 1.0), Vector2(0.0, 0.0) }, // C 2
			{ Vector3( 0.5,  0.5,  0.5), Vector3(1.0, 1.0, 0.0), Vector2(0.0, 0.0) }, // D 3
			{ Vector3( 0.5, -0.5, -0.5), Vector3(1.0, 0.0, 1.0), Vector2(0.0, 0.0) }, // E 4
			{ Vector3(-0.5, -0.5, -0.5), Vector3(0.0, 1.0, 1.0), Vector2(0.0, 0.0) }, // F 5
			{ Vector3(-0.5,  0.5, -0.5), Vector3(1.0, 1.0, 1.0), Vector2(0.0, 0.0) }, // G 6
			{ Vector3( 0.5,  0.5, -0.5), Vector3(0.0, 0.0, 0.0), Vector2(0.0, 0.0) }, // H 7
		},
		{
			// top
			0, 1, 2,
			0, 2, 3,
			// bottom
			4, 6, 5,
			4, 7, 6,
			// right
			2, 6, 7,
			3, 2, 7,
			// front
			0, 3, 4,
			4, 3, 7,
			// back
			2, 1, 5,
			2, 5, 6,
			// left
			1, 0, 4,
			1, 4, 5,
		}
	);
}
Mesh Mesh::create_triangle()
{
	return create_simple_mesh(
		{
			{ { -0.5f, -0.5f,  0.0f }, { 1.f, 0.f, 0.f }, { 0.f, 0.f } },
			{ {  0.5f,  0.0f,  0.0f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f } },
			{ { -0.5f,  0.5f,  0.0f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f } },
		},
		{
			0, 1, 2,
			0, 2, 1,
		}
	);
}