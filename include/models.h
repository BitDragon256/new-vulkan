#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "nve_types.h"
#include "buffer.h"
#include "math-core.h"

class Texture;

class Material
{
public:
	Texture* m_pDiffuseTexture;
	Texture* m_pNormalTexture;
	VkShaderModule* m_pShader;
	glm::vec4 m_diffuseColor;
	Vector3 m_ambientColor;
	float m_alphaTest;
};

class Submesh
{
public:
	Material m_material;

	std::vector<Vertex> vertices;
	std::vector<Index> indices;
};

class Mesh
{
public:
	std::vector<Submesh> m_submeshes;

	void write_vertíces_to(std::vector<Vertex>::iterator dst);
	void write_indices_to(std::vector<Index>::iterator dst);

	size_t vertex_count();
	size_t index_count();

	static Mesh create_simple_mesh(std::vector<Vertex> vertices, std::vector<Index> indices);
	static Mesh create_cube();
	static Mesh create_triangle();
};

struct Transform
{
	Vector3 position;
	Vector3 scale;
	Quaternion rotation;
};

class Model
{
public:
	std::vector<Mesh> m_meshes;
	Transform m_info;

	size_t vertex_count();
	size_t index_count();

	void write_vertíces_to(std::vector<Vertex>::iterator dst);
	void write_indices_to(std::vector<Index>::iterator dst);

	static Model create_model(Mesh mesh);
};

class ModelHandler
{
public:
	Buffer<Vertex> m_vertexBuffer;
	Buffer<Index> m_indexBuffer;
	Buffer<Transform> m_modelBuffer;

	std::vector<Model*> m_models;

	ModelHandler();
	void init(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool cmdPool, VkQueue queue);
	void destroy_buffers();
	void upload_data();
	void upload_model_info();
	void add_model(Model* pModel);
	VkBuffer vertex_buffer();
	VkBuffer index_buffer();
	VkBuffer model_buffer();
	uint32_t vertex_count();
	uint32_t index_count();

private:
	void reset();

	VkDevice m_device;
	VkPhysicalDevice m_physicalDevice;
	VkCommandPool m_commandPool;
	VkQueue m_transferQueue;

	size_t m_vertexCount;
	size_t m_indexCount;

	bool m_initialized;
	bool m_dataChanged;
};