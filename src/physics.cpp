#include "physics.h"

#include "tritri.h"

const float Epsilon = 0.0001f;

bool operator> (Vector3 a, Vector3 b)
{
	return a.x > b.x && a.y > b.y && a.z > b.z;
}

bool colliding(AxisAlignedBoundingBox a, AxisAlignedBoundingBox b)
{
	return a.max > b.min && b.max > a.min;
}

void PhysicsSystem::awake(EntityId entity)
{
	reset_rigidbody(entity);
}
void PhysicsSystem::update(float dt)
{
	sync_rigidbody();

	physics_tick(dt);

	sync_transform();
}
void PhysicsSystem::physics_tick(float dt)
{
	for (EntityId entity : m_entities)
	{
		auto& rb = get_rigidbody(entity);
		rb.acc = calc_acc(entity, rb.pos);
	}
	for (EntityId entity : m_entities)
	{
		integrate(entity, dt);
	}
}
void PhysicsSystem::solve_collision(EntityId a, EntityId b)
{
	auto& rbA = get_rigidbody(a); auto& rbB = get_rigidbody(b);
	Vector3 ab = glm::normalize(rbB.pos - rbA.pos);
	float force = collision_force(a, b);
	rbA.pos -= 0.5f * ab * force;
	rbB.pos += 0.5f * ab * force;
}
float PhysicsSystem::collision_force(EntityId a, EntityId b)
{
	return intersection_length(a, b);
}
bool PhysicsSystem::colliding(EntityId a, EntityId b)
{
	return intersection_length(a, b) > 0;
}
float PhysicsSystem::intersection_length(EntityId a, EntityId b)
{
	const auto& rbA = get_rigidbody(a); const auto& rbB = get_rigidbody(b);

	return (rbA.radius + rbB.radius) - glm::length(rbA.pos - rbB.pos);
}
void PhysicsSystem::ball_constraint(EntityId entity)
{
	auto& rb = get_rigidbody(entity);

	float intersectionDist = -(CONSTRAINT_BALL_RADIUS - (glm::length(rb.pos) + rb.radius));
	Vector3 normal = glm::normalize(rb.pos);
	if (intersectionDist > 0)
	{
		rb.pos -= normal * intersectionDist;
	}
}
Vector3 PhysicsSystem::calc_acc(EntityId entity, Vector3 pos)
{
	Vector3 acc = { 0,0,0 };

	// collision with other rigidbody
	for (auto b = std::find(m_entities.begin(), m_entities.end(), entity) + 1; b != m_entities.end(); b++)
	{
		if (!colliding(entity, *b) || entity == *b)
			continue;
		solve_collision(entity, *b);
	}

	// keep constraint
	ball_constraint(entity);

	// gravity
	acc += VECTOR_DOWN * GRAVITY_FORCE;

	return acc;
}
void PhysicsSystem::integrate(EntityId entity, float dt)
{
	classic_verlet(entity, dt);
}
void PhysicsSystem::velocity_verlet(EntityId entity, float dt)
{
	auto& rb = get_rigidbody(entity);

	Vector3 nextPos = rb.pos + rb.vel * dt + 0.5f * rb.acc * dt * dt;
	Vector3 nextAcc = calc_acc(entity, nextPos);
	Vector3 nextVel = rb.vel + 0.5f * (rb.acc + nextAcc) * dt;

	rb.pos = nextPos;
	rb.vel = nextVel;
	rb.acc = nextAcc;
}
void PhysicsSystem::classic_verlet(EntityId entity, float dt)
{
	auto& rb = get_rigidbody(entity);

	Vector3 nextPos = 2.f * rb.pos - rb.lastPos + 0.5f * rb.acc * dt * dt;

	rb.lastPos = rb.pos;
	rb.pos = nextPos;

	// for informatory reasons
	auto vel = (rb.lastPos - rb.pos) / dt;
	rb.acc = (rb.vel - vel) / dt;
	rb.vel = vel;
}

Rigidbody& PhysicsSystem::get_rigidbody(EntityId entity)
{
	return m_ecs->get_component<Rigidbody>(entity);
}
Transform& PhysicsSystem::get_transform(EntityId entity)
{
	return m_ecs->get_component<Transform>(entity);
}

void PhysicsSystem::sync_transform()
{
	for (EntityId entity : m_entities)
	{
		sync_transform(entity);
	}
}
void PhysicsSystem::sync_rigidbody()
{
	for (EntityId entity : m_entities)
	{
		sync_rigidbody(entity);
	}
}
void PhysicsSystem::sync_transform(EntityId entity)
{
	get_transform(entity).position = get_rigidbody(entity).pos;
}
void PhysicsSystem::sync_rigidbody(EntityId entity)
{
	get_rigidbody(entity).pos = get_transform(entity).position;
}
void PhysicsSystem::reset_rigidbody(EntityId entity)
{
	sync_rigidbody(entity);
	get_rigidbody(entity).lastPos = get_rigidbody(entity).pos;
}

Triangle make_tri()
{
	Triangle tri;

	tri.mesh = nullptr;
	tri.index = 0;
	tri.a = { 0, 0, 0 };
	tri.b = { 0, 0, 0 };
	tri.c = { 0, 0, 0 };

	return tri;
}
Triangle make_tri(Vector3 a, Vector3 b, Vector3 c)
{
	Triangle tri;

	tri.mesh = nullptr;
	tri.index = 0;
	tri.a = a;
	tri.b = b;
	tri.c = c;

	return tri;
}
RayhitInfo make_hit_info(Vector3 start, Vector3 direction)
{
	RayhitInfo hit;

	hit.start = start;
	hit.direction = direction;
	hit.impact = { 0, 0, 0 };
	hit.dist = std::numeric_limits<float>::max();
	hit.hit = false;
	hit.rb = nullptr;
	hit.tri = make_tri();

	return hit;
}

void intersect_tri_ray(RayhitInfo& hit)
{
	Vector3 e1, e2, h, s, q;
	float a, f, u, v;

	e1 = hit.tri.b - hit.tri.a;
	e2 = hit.tri.c - hit.tri.a;
	h = glm::cross(hit.direction, e2);
	a = glm::dot(e1, h);

	// parallel
	if (a > -Epsilon && a < Epsilon)
		return;

	f = 1.f / a;
	s = hit.start - hit.tri.a;
	u = f * glm::dot(s, h);

	// no idea
	if (u < 0.f || u > 1.f)
		return;

	q = glm::cross(s, e1);
	v = f * glm::dot(hit.direction, q);

	// just think up something
	if (v < 0.f || u + v > 1.f)
		return;

	float t = f * glm::dot(e2, q);

	// hit
	if (t > Epsilon)
	{
		hit.hit = true;
		hit.impact = hit.start + hit.direction * t;
		hit.dist = glm::length(hit.impact - hit.start);

		return;
	}
	else // line intersection, no ray intersection
		return;
}

bool PhysicsSystem::raycast(Vector3 start, Vector3 direction, RayhitInfo* hitInfo, float maxDist)
{
	RayhitInfo hit = make_hit_info(start, direction);

	for (EntityId entity : m_entities)
	{
		auto& rb = get_rigidbody(entity);

		for (size_t triIndex = 0; triIndex < rb.mesh.indices.size(); triIndex += 3)
		{
			hit.tri = make_tri();
			hit.tri.a = rb.mesh.vertices[triIndex + 0].pos;
			hit.tri.b = rb.mesh.vertices[triIndex + 1].pos;
			hit.tri.c = rb.mesh.vertices[triIndex + 2].pos;

			intersect_tri_ray(hit);

			if (hit.hit)
			{
				hit.rb = &rb;

				if (hitInfo)
					*hitInfo = hit;

				return true;
			}
		}
	}

	if (hitInfo)
		*hitInfo = hit;

	return false;
}

float intersect_edge_edge(Vector3 p, Vector3 q, Vector3 v, Vector3 w)
{
	int is[] = { 0, 0, 1, 1, 2, 2 };
	int js[] = { 1, 2, 0, 2, 0, 1 };

	int i, j;
	float divider;

	bool valid = false;
	for (int index = 0; index < 6; index++)
	{
		i = is[index]; j = js[index];
		divider = q[i] * w[j] - v[i] * q[j];
		if (abs(divider) <= Epsilon)
			continue;
		valid = true;
		break;
	}
	if (!valid)
		return std::numeric_limits<float>::max();

	return (v[i] - p[i] + v[i] * (p[j] - v[j])) / divider;
}

bool same_dir(Vector3 a, Vector3 b)
{
	Vector3 diff = a - b;
	return glm::dot(diff, diff) < Epsilon;
}

float area_tri(Triangle tri)
{
	// calculate area after heron
	Vector3 edges = {
		glm::length(tri.b - tri.a),
		glm::length(tri.c - tri.a),
		glm::length(tri.c - tri.b)
	};
	float s = 0.5f * (edges.x + edges.y + edges.z);
	return sqrt(s * (s - edges.x) * (s - edges.y) * (s - edges.z));
}

#define FALSE_VECTOR Vector3{ 10000, 10000, 10000 }
Vector3 intersect_edge_plane(Vector3 u, Vector3 v, Vector3 p, Vector3 q, Vector3 o)
{
	Vector3 n = glm::cross(q, o);

	float divisor = glm::dot(v, n);
	if (divisor == 0)
		return FALSE_VECTOR;

	float t = glm::dot(u - p, n) / divisor * -1;
	return u + t * v;
}

bool tri_contains_point(Triangle tri, Vector3 point)
{
	float triArea = area_tri(tri);

	float totalArea = 0;
	totalArea += area_tri(make_tri(point, tri.a, tri.b));
	totalArea += area_tri(make_tri(point, tri.a, tri.c));
	totalArea += area_tri(make_tri(point, tri.b, tri.c));

	return totalArea <= triArea + Epsilon;
}

bool intersect_ray_tri(Vector3 start, Vector3 dir, Triangle tri, Vector3& out)
{
	Vector3 vertex0 = tri.a;
	Vector3 vertex1 = tri.b;
	Vector3 vertex2 = tri.c;
	Vector3 edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	h = glm::cross(dir, edge2);
	a = glm::dot(edge1, h);

	if (a > -Epsilon && a < Epsilon)
		return false;    // This ray is parallel to this triangle.

	f = 1.0 / a;
	s = start - vertex0;
	u = f * glm::dot(s, h);

	if (u < 0.0 || u > 1.0)
		return false;

	q = glm::cross(s, edge1);
	v = f * glm::dot(dir, q);

	if (v < 0.0 || u + v > 1.0)
		return false;

	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * glm::dot(edge2, q);

	if (t > Epsilon) // ray intersection
	{
		out = start + dir * t;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
}

#define VEC2FLOAT(V) {V.x, V.y, V.z}

void intersect_tri_tri(Triangle a, Triangle b, TriangleIntersection& info)
{
	float v0[3] = VEC2FLOAT(a.a), v1[3] = VEC2FLOAT(a.b), v2[3] = VEC2FLOAT(a.c),
		  u0[3] = VEC2FLOAT(b.a), u1[3] = VEC2FLOAT(b.b), u2[3] = VEC2FLOAT(b.c);
	int intersect = tri_tri_intersect(v0, v1, v2, u0, u1, u2);

	info.intersect = intersect;
//	info.intersect = false;
//	info.start = { 0,0,0 };
//	info.end = { 0,0,0 };
//
//	/*auto U = a.b - a.a;
//	auto V = a.c - a.a;
//	auto S = b.b - b.a;
//	auto T = b.c - b.a;
//
//	auto UxV = glm::cross(U, V);
//	float sigma = glm::dot(UxV, UxV);
//	Vector3 alpha = glm::cross(S, UxV) / sigma;
//	Vector3 beta = glm::cross(T, UxV) / sigma;
//	Vector3 gamma = glm::cross(b.a - a.a, UxV) / sigma;
//*/
//	float areaA = area_tri(a);
//	float areaB = area_tri(b);
//
//	std::array<Vector3, 6> edgesA = {
//		a.a, a.b - a.a,
//		a.a, a.c - a.a,
//		a.b, a.c - a.b
//	};
//	std::array<Vector3, 6> edgesB = {
//		b.a, b.b - b.a,
//		b.a, b.c - b.a,
//		b.b, b.c - b.b
//	};
//
//	// intersect edges of a with b
//	for (int i = 0; i < 6; i += 2)
//	{
//		Vector3 intersect;
//		if (!intersect_ray_tri(edgesA[i], edgesA[i + 1], b, intersect))
//			continue;
//
//		// successful intersection
//		info.intersect = true;
//		return;
//	}
//
//	// intersect edges of b with a
//	for (int i = 0; i < 6; i += 2)
//	{
//		Vector3 intersect;
//		if (!intersect_ray_tri(edgesB[i], edgesB[i + 1], a, intersect))
//			continue;
//
//		// successful intersection
//		info.intersect = true;
//		return;
//	}
}