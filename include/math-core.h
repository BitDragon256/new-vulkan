#pragma once

#include "nve_types.h"

#define DEG_TO_RAD PI / 180.f
#define RAD_TO_DEG 180.f / PI

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

		Vector3 to_euler() const;
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