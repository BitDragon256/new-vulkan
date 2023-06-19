#pragma once

#include "nve_types.h"

namespace math
{
	class quaternion
	{
	public:
		quaternion(); // identity
		quaternion(Vector3 euler); // construct from euler angles
		quaternion(Vector4 v); // construct from (x, y, z, w) vector
		quaternion(float angle, Vector3 axis); // quaternion to rotate angle (radians) around axis
		quaternion(const quaternion& quaternion); // copy constructor

		float i;
		float j;
		float k;
		float w;

		void normalize();
		quaternion normalized();

		void euler(Vector3 euler);
		void set(float i, float j, float k, float w);
		void set(Vector3 complex, float real);
	};
} // namespace math