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
	quaternion::quaternion(Vector3 euler)
	{

	}
	quaternion::quaternion(Vector4 v)
	{
		set(v.x, v.y, v.z, v.w);
	}
	quaternion::quaternion(float angle, Vector3 axis)
	{
		w = cos(angle);
		float sine = sin(angle);
		axis = glm::normalize(axis) * sine;

		set(axis, w);
	}
	quaternion::quaternion(const quaternion& quaternion)
	{
		set(quaternion.i, quaternion.j, quaternion.k, quaternion.w);
	}

	// --------------------------------
	// METHODS
	// --------------------------------

	void quaternion::set(float i, float j, float k, float w)
	{
		this->i = i;
		this->j = j;
		this->k = k;
		this->w = w;
	}
	void quaternion::set(Vector3 complex, float real)
	{
		set(complex.x, complex.y, complex.z, real);
	}

} // namespace math