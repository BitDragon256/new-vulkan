#pragma once

#include "nve_types.h"

namespace math
{
	class quaternion
	{
	public:
		quaternion(); // identity
		quaternion(float x, float y, float z, float w); // basic initialization
		quaternion(Vector3 euler); // construct from euler angles
		quaternion(Vector4 v); // construct from (x, y, z, w) vector
		quaternion(float angle, Vector3 axis); // quaternion to rotate angle (radians) around axis
		quaternion(const quaternion& quaternion); // copy constructor

		float x;
		float y;
		float z;
		float w;

		float length();
		float sqr_mag();

		void normalize();
		quaternion normalized();
		quaternion conjugated();
		Vector3 vector_part();

		void euler(Vector3 euler);
		void set(float i, float j, float k, float w);
		void set(Vector3 complex, float real);

		quaternion operator*(const quaternion& other);

		static Vector3 rotate(Vector3 vec, quaternion quat);
	
	private:
		bool is_normalized();
		static const float m_normalizeThreshold;
	};

	float abs(float x);
	float min(float a, float b);
	float max(float a, float b);
	float clamp(float x, float lower, float upper);

} // namespace math