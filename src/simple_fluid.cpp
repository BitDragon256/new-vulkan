#include "simple_fluid.h"

#include <math.h>
#include <time.h>

#include "logger.h"

void SimpleFluid::awake(EntityId id)
{
	Particle& particle = get_particle(id);

	particle.index = m_pIndex;
	m_pIndex++;
}
void SimpleFluid::update(float dt)
{
	cache_densities();

	for (auto pId : m_entities)
	{
		Particle& particle = get_particle(pId);

		particle.velocity += pressure_force(particle) * dt;
		particle.position += particle.velocity * dt;
		bounds_check(particle);

		m_ecs->get_component<Transform>(pId).position = { particle.position.x, particle.position.y, 0 };
	}
}
void SimpleFluid::update(float dt, EntityId id)
{
	/*Particle& particle = get_particle(id);

	particle.velocity += pressure_force(particle) * dt;
	particle.position += particle.velocity * dt;
	bounds_check(particle);

	m_ecs->get_component<Transform>(id).position = { particle.position.x, particle.position.y, 0 };*/
}

Vector2 SimpleFluid::pressure_force(Particle& particle)
{
	Vector2 force{ 0,0 };
	for (EntityId pId : m_entities)
	{
		Particle& p = get_particle(pId);
		if (particle.index == p.index)
			continue;

		Vector2 dir = p.position - particle.position;
		float dist = glm::length(dir);
		dir = dir / dist;
		float density = m_densities[p.index];
		float sharedPressure = (density_to_pressure(density) + density_to_pressure(m_densities[particle.index])) / 2.f;
		Vector2 dF = -sharedPressure * dir * influence_slope(m_smoothingRadius, dist) / density;
		force += dF;
	}
	return force;
}
float SimpleFluid::influence(float rad, float d)
{
	if (d > rad)
		return 0;
	return powf(rad - d, 3) / influence_volume(rad);
}
float SimpleFluid::influence_slope(float rad, float d)
{
	if (d > rad)
		return 0;
	return -6.f * d * powf(rad - d, 2.f) / (PI * powf(rad, 4.f));
}
float SimpleFluid::influence_volume(float rad)
{
	return PI * powf(rad, 4.f) / 2.f;
}
float SimpleFluid::density_to_pressure(float density)
{
	return (density - m_targetDensity) * m_pressureMultiplier;
}

void SimpleFluid::cache_densities()
{
	if (m_densities.size() < m_entities.size())
		m_densities.resize(m_entities.size());

	for (EntityId pId : m_entities)
	{
		Particle& p = get_particle(pId);
		m_densities[p.index] = density_at(p.position);
	}
}
float SimpleFluid::density_at(Vector2 position)
{
	float density{ 0 };
	for (EntityId pId : m_entities)
	{
		Particle& p = get_particle(pId);
		float d = glm::length(p.position - position);
		density += influence(m_smoothingRadius, d);
	}
	return density;
}

Particle& SimpleFluid::get_particle(EntityId id)
{
	return m_ecs->get_component<Particle>(id);
}
void SimpleFluid::bounds_check(Particle& particle)
{
	if (particle.position.x < m_minBounds.x || particle.position.x > m_maxBounds.x)
		particle.velocity.x *= -1;
	if (particle.position.y < m_minBounds.y || particle.position.y > m_maxBounds.y)
		particle.velocity.y *= -1;
}