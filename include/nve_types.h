#pragma once

#include <array>

#include <glm/glm.hpp>

#include "vulkan_helpers.h"

typedef int NVE_RESULT;

// success codes
#define NVE_SUCCESS 0
#define NVE_RENDER_EXIT_SUCCESS 100

// error codes
#define NVE_FAILURE -1

typedef uint32_t Index;
#define NVE_INDEX_TYPE VK_INDEX_TYPE_UINT32

#define VERTEX_ATTRIBUTE_COUNT 4

namespace math
{
	class quaternion;
};

typedef glm::vec2 Vector2;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;
typedef math::quaternion Quaternion;
typedef Vector3 Color;

typedef struct Vertex
{
	Vector3 pos;
	Vector3 normal;
	Vector3 color;
	Vector2 uv;
	uint32_t material;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, VERTEX_ATTRIBUTE_COUNT> getAttributeDescriptions();

	bool operator== (const Vertex& other) const;
} Vertex;

struct CameraPushConstant
{
	glm::mat4 view;
	glm::mat4 proj;
};

#define VECTOR_UP Vector3(0, 0, 1)
#define VECTOR_FORWARD Vector3(1, 0, 0)
#define VECTOR_RIGHT Vector3(0, 1, 0)

#define VECTOR_DOWN Vector3(0, 0, -1)
#define VECTOR_BACK Vector3(-1, 0, 0)
#define VECTOR_LEFT Vector3(0, -1, 0)

#define PI 3.14159265