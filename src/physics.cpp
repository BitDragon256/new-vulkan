#include "physics.h"

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