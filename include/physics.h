#pragma once

#include <limits>
#include <sstream>

struct Rigidbody;

#include "ecs.h"
#include "nve_types.h"
#include "model-handler.h"

#include "gui.h"
//#include "render.h"

#define GRAVITY_FORCE 5.f
#define CONSTRAINT_BALL_RADIUS 2.f

// testing with spheres
struct Rigidbody
{
	float radius;
	Vector3 pos;
	Vector3 lastPos;
	Vector3 vel;
	Vector3 acc;

	float mass;
	Mesh mesh;
};

GUI_PRINT_COMPONENT_START(Rigidbody)

ImGui_DragVector("position", component.pos);
ImGui_DragVector("velocity", component.vel);
ImGui_DragVector("acceleration", component.acc);
ImGui::DragFloat("mass", &component.mass);

GUI_PRINT_COMPONENT_END

struct Triangle
{
	Mesh* mesh;
	uint32_t index;

	Vector3 a, b, c;
};

struct RayhitInfo
{
	Vector3 start;
	Vector3 direction;
	Vector3 impact;
	float dist;
	bool hit;
	Rigidbody* rb;
	Triangle tri;
};

struct TriangleIntersection
{
	bool intersect;

	// line of intersection
	Vector3 start;
	Vector3 end;
};

struct AxisAlignedBoundingBox
{
	Vector3 min;
	Vector3 max;
};
bool colliding(AxisAlignedBoundingBox a, AxisAlignedBoundingBox b);

class PhysicsSystem : System<Transform, Rigidbody>
{
public:
	void awake(EntityId entity) override;
	void update(float dt) override;

	bool raycast(Vector3 start, Vector3 direction, RayhitInfo* hitInfo = nullptr, float maxDist = std::numeric_limits<float>::max());
private:
	void sync_transform();
	void sync_rigidbody();
	void sync_transform(EntityId entity);
	void sync_rigidbody(EntityId entity);
	void reset_rigidbody(EntityId entity);

	void physics_tick(float dt);
	void solve_collision(EntityId a, EntityId b);
	float collision_force(EntityId a, EntityId b);
	bool colliding(EntityId a, EntityId b);
	float intersection_length(EntityId a, EntityId b);
	Vector3 calc_acc(EntityId, Vector3 pos);
	void integrate(EntityId entity, float dt);

	void ball_constraint(EntityId entity);

	void velocity_verlet(EntityId entity, float dt);
	void classic_verlet(EntityId entity, float dt);

	Rigidbody& get_rigidbody(EntityId entity);
	Transform& get_transform(EntityId entity);
};

class SpacialAccelerationStructure
{
public:
	void set_point(EntityId entity, Vector3 point);
	EntityId get_nearest(Vector3 point, std::vector<EntityId> ignore = {});
	std::vector<EntityId> get_all_nearest(Vector3 point, std::vector<EntityId> ignore = {});

private:
	std::unordered_map<EntityId, Vector3> m_points;
};