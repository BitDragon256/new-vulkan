#include "math-core.h"

#include <math.h>

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

		normalize();
	}
	void quaternion::set(Vector3 complex, float real)
	{
		set(complex.x, complex.y, complex.z, real);
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
		normalize();
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

} // namespace math