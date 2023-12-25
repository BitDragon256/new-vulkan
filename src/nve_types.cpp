#include "nve_types.h"

// ------------------------------------------
// TRANSFORM
// ------------------------------------------

Transform::Transform() :
	position{ VECTOR_NULL }, scale{ 1, 1, 1 }, rotation{}, materialStart(0)
{}
Transform::Transform(Vector3 position, Vector3 scale, Quaternion rotation) :
	position{ position }, scale{ scale }, rotation{ rotation }, materialStart{ 0 }
{}

bool operator== (const Transform& a, const Transform& b)
{
	return
		a.position == b.position &&
		a.scale == b.scale &&
		a.rotation == b.rotation &&
		a.materialStart == b.materialStart;
}

