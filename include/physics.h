#pragma once

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
};

GUI_PRINT_COMPONENT_START(Rigidbody)

std::stringstream ss;
ss << "pos: " << component.pos
<< "vel: " << component.vel
<< "acc: " << component.acc
<< "mass: " << component.mass;
return ss.str();

GUI_PRINT_COMPONENT_END

class PhysicsSystem : System<Transform, Rigidbody>
{
public:
	void awake(EntityId entity) override;
	void update(float dt) override;
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