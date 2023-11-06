#pragma once

#include <vector>

#include "nve_types.h"
#include "ecs.h"
#include "model-handler.h"

struct Particle
{
	Vector2 position;
	Vector2 velocity;
	size_t index;
};

class SimpleFluid : System<Particle, Transform>
{
public:
	float m_smoothingRadius = 3.f;
	float m_targetDensity = 2.f;
	float m_pressureMultiplier = 100.f;
	const Vector2 m_maxBounds = { 15, 15 };
	const Vector2 m_minBounds = { -15, -15 };
	void awake(EntityId) override;
	void update(float dt) override;
	void update(float dt, EntityId id) override;

private:
	float influence(float rad, float d);
	float influence_slope(float rad, float d);
	float influence_volume(float rad);
	float density_to_pressure(float density);
	Vector2 pressure_force(Particle& particle);
	void cache_densities();
	float density_at(Vector2 position);

	size_t m_pIndex = 0;
	std::vector<float> m_densities;

	// Helper Functions
	Particle& get_particle(EntityId id);
	void bounds_check(Particle& particle);
};