#include "math-core.h"

#include <math.h>

#include <glm/gtc/quaternion.hpp>

namespace math
{

	// ---------------------------------
	// quaternion
	// --------------------------------

	// --------------------------------
	// CONSTRUCTORS
	// --------------------------------

	quaternion::quaternion()
	{
		set(0, 0, 0, 1);
	}
	quaternion::quaternion(float x, float y, float z, float w)
	{
		set(x, y, z, w);
	}
	quaternion::quaternion(Vector3 euler)
	{
		quaternion x = quaternion(euler.x, VECTOR_FORWARD);
		quaternion y = quaternion(euler.y, VECTOR_RIGHT);
		quaternion z = quaternion(euler.z, VECTOR_UP);
		*this = x * y * z;
	}
	quaternion::quaternion(Vector4 v)
	{
		set(v.x, v.y, v.z, v.w);
	}
	quaternion::quaternion(float angle, Vector3 axis)
	{
		angle *= DEG_TO_RAD;
		if (angle > 2 * PI)
			angle -= ((int)angle / (2 * PI)) * 2 * PI;
		angle /= 2;

		w = cos(angle);
		float sine = sin(angle);
		axis = glm::normalize(axis) * sine;

		set(axis, w);
	}
	quaternion::quaternion(const quaternion& quaternion)
	{
		set(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
	}

	// --------------------------------
	// METHODS
	// --------------------------------

	float quaternion::sqr_mag()
	{
		return x * x + y * y + z * z + w * w;
	}
	float quaternion::length()
	{
		return sqrt(sqr_mag());
	}

	void quaternion::normalize()
	{
		if (is_normalized())
			return;

		float len = length();
		x /= len;
		y /= len;
		z /= len;
		w /= len;
	}
	quaternion quaternion::normalized()
	{
		quaternion quat(*this);
		quat.normalize();
		return quat;
	}
	quaternion quaternion::conjugated()
	{
		return quaternion(-x, -y, -z, w);
	}
	Vector3 quaternion::vector_part()
	{
		return Vector3(x, y, z);
	}

	void quaternion::set(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	void quaternion::set(Vector3 complex, float real)
	{
		set(complex.x, complex.y, complex.z, real);
	}
	void quaternion::euler(Vector3 rot)
	{
		*this = quaternion(rot);
	}

	Vector3 quaternion::to_euler() const
	{
		Vector3 angles;

		// roll (x-axis rotation)
		double sinr_cosp = 2 * (w * x + y * x);
		double cosr_cosp = w * w + z * z - y * y - x * x;
		angles.x = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		double sinp = std::sqrt(1 + 2 * (w * y - x * z));
		double cosp = std::sqrt(1 - 2 * (w * y - x * z));
		angles.y = 2 * std::atan2(sinp, cosp) - PI / 2;

		// yaw (z-axis rotation)
		double siny_cosp = 2 * (w * z + x * y);
		double cosy_cosp = 1 - 2 * (y * y + z * z);
		angles.z = std::atan2(siny_cosp, cosy_cosp);

		return angles * RAD_TO_DEG;
	}

	const float quaternion::m_normalizeThreshold { 0.05f };
	bool quaternion::is_normalized()
	{
		return abs(sqr_mag() - 1) < m_normalizeThreshold;
	}

	// --------------------------------
	// OPERATORS
	// --------------------------------

	quaternion quaternion::operator*(const quaternion& q)
	{
		quaternion ret;
		ret.x = (w*q.x + x*q.w + y*q.z - z*q.y);
		ret.y = (w*q.y - x*q.z + y*q.w + z*q.x);
		ret.z = (w*q.z + x*q.y - y*q.x + z*q.w);
		ret.w = (w*q.w - x*q.x - y*q.y - z*q.z);

		return ret;
	}

	// --------------------------------
	// STATIC FUNCTIONS
	// --------------------------------

	Vector3 quaternion::rotate(Vector3 vec, quaternion quat)
	{
		quaternion t(vec.x, vec.y, vec.z, 0);

		return (quat * t * quat.conjugated()).vector_part();
	}

	// --------------------------------
	// FUNCTIONS
	// --------------------------------
	float abs(float x)
	{
		return x * ((x > 0) - (x < 0));
	}
	float min(float a, float b)
	{
		return (a + b - abs(a - b)) / 2;
	}
	float max(float a, float b)
	{
		return (a + b + abs(a - b)) / 2;
	}

	float clamp(float x, float lower, float upper)
	{
		return min(max(x, lower), upper);
	}

} // namespace math