#pragma once

#include <glm/glm.hpp>

typedef int NVE_RESULT;

namespace math
{
	class quaternion;
};

typedef glm::vec2 Vector2;
typedef glm::vec3 Vector3;
typedef glm::vec4 Vector4;
typedef math::quaternion Quaternion;
typedef Vector3 Color;

struct Vertex;
struct CameraPushConstant;
struct Mesh;
struct Transform;
