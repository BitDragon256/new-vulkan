#pragma once

#include <array>

#include <glm/glm.hpp>

typedef int NVE_RESULT;

// success codes
#define NVE_SUCCESS 0
#define NVE_RENDER_EXIT_SUCCESS 100

// error codes
#define NVE_FAILURE -1

typedef uint32_t Index;
#define NVE_INDEX_TYPE VK_INDEX_TYPE_UINT32

#define VERTEX_ATTRIBUTE_COUNT 3

typedef struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> getAttributeDescriptions();
} Vertex;

#define VECTOR_UP glm::vec3(0, 0, 1)
#define VECTOR_FORWARD glm::vec3(1, 0, 0)
#define VECTOR_RIGHT glm::vec3(0, 1, 0)

#define VECTOR_DOWN glm::vec3(0, 0, -1)
#define VECTOR_BACK glm::vec3(-1, 0, 0)
#define VECTOR_LEFT glm::vec3(0, -1, 0)

#define PI 3.14159265