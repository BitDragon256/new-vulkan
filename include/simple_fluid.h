#pragma once

#include <vector>

#include "nve_types.h"
#include "ecs.h"
#include "model-handler.h"
#include "profiler.h"

struct Particle
{
	Vector2 position;
	//Vector2 velocity;
	Vector2 lastPosition;
	Vector2 acc;
	size_t index;
};

#define SF_BOUNDING_WIDTH 16
#define SF_BOUNDING_HEIGHT 9

class SimpleFluid : System<Particle, Transform>
{
public:
	SimpleFluid();

	bool m_active;
	float m_gravity = 0.0f;
	float m_smoothingRadius = .3f;
	float m_targetDensity = 1.f;
	float m_pressureMultiplier = 5.f;
	float m_wallForceMultiplier = 1.f;
	float m_collisionDamping = 4.f;
	const Vector2 m_maxBounds = { SF_BOUNDING_HEIGHT / 2.f, SF_BOUNDING_WIDTH / 2.f };
	const Vector2 m_minBounds = { -SF_BOUNDING_HEIGHT / 2.f, -SF_BOUNDING_WIDTH / 2.f };
	void awake(EntityId) override;
	void update(float dt) override;
	void update(float dt, EntityId id) override;
	void gui_show_system() override;

private:
	std::vector<Particle*> m_particles;

	float influence(float rad, float d);
	float influence_slope(float rad, float d);
	float influence_volume(float rad);
	float density_to_pressure(float density);
	Vector2 pressure_force(Particle& particle, Vector2 predictedPosition);
	float wall_force(float d);
	void cache_densities();
	float density_at(Vector2 position);

	size_t m_pIndex = 0;
	std::vector<float> m_densities;

	// integration
	void integrate(Vector2& position, Vector2& lastPosition, const Vector2& acceleration, float dt);
	void classic_verlet(Vector2& position, Vector2& lastPosition, const Vector2& acceleration, float dt);

	// spatial hashing
	std::vector<std::vector<Particle*>> m_buckets;
	const float m_bucketSize = .3f;
	uint32_t m_yBuckets;
	void create_buckets();
	uint32_t pos_to_bucket_index(Vector2 pos);
	std::vector<Particle*>& bucket_at(Vector2 pos);
	std::vector<Particle*> surrounding_buckets(Vector2 pos);

	// Helper Functions
	Particle& get_particle(EntityId id);
	bool bounds_check(Particle& particle);

	Profiler m_profiler;
};