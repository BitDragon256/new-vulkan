#include "simple_fluid.h"

#include <math.h>
#include <time.h>

#include "logger.h"

#define SIMPLE_FLUID_PROFILER
#undef SIMPLE_FLUID_PROFILER

SimpleFluid::SimpleFluid()
{
	create_buckets();
}
void SimpleFluid::awake(EntityId id)
{
	Particle& particle = get_particle(id);
	m_particles.push_back(&particle);
	bucket_at(particle.position).push_back(&particle);

	particle.lastPosition = particle.position;

	particle.index = m_pIndex;
	m_pIndex++;
}
void SimpleFluid::update(float dt)
{
	if (!m_active)
		return;

#ifdef SIMPLE_FLUID_PROFILER
	m_profiler.start_measure("cache densities");
#endif
	cache_densities();
#ifdef SIMPLE_FLUID_PROFILER
	m_profiler.end_measure("cache densities", true);
#endif

	float avgPressureForce{ 0 }, avgBucketMv{ 0 };

	size_t i = 0;
	#pragma omp parallel for
	for (auto particlePtr : m_particles)
	{
		Particle& particle = *particlePtr;
		particle.acc = Vector2(0);

#ifdef SIMPLE_FLUID_PROFILER
		m_profiler.start_measure("pressure force");
#endif

		Vector2 predictedPos = particle.position, lastPos = particle.lastPosition;
		particle.acc += Vector2 { m_gravity, 0 };

		integrate(predictedPos, lastPos, particle.acc, dt);

		//particle.velocity += pressure_force(particle) * dt;
		particle.acc += glm::clamp(pressure_force(particle, predictedPos), Vector2(-15.f), Vector2(15.f));

#ifdef SIMPLE_FLUID_PROFILER
		avgPressureForce += m_profiler.end_measure("pressure force");
#endif

#ifdef SIMPLE_FLUID_PROFILER
		m_profiler.start_measure("bucket move");
#endif

		auto& bucket = bucket_at(particle.position);
		bucket.erase(std::find(bucket.begin(), bucket.end(), particlePtr));

		integrate(particle.position, particle.lastPosition, particle.acc, dt);

		if (bounds_check(particle));
		//	particle.position += particle.velocity * dt;
		bucket_at(particle.position).push_back(particlePtr);

#ifdef SIMPLE_FLUID_PROFILER
		avgBucketMv += m_profiler.end_measure("bucket move");
#endif

		// sync pos with transform
		m_ecs->get_component<Transform>(m_entities[i++]).position = {particle.position.x, particle.position.y, 0};
	}

#ifdef SIMPLE_FLUID_PROFILER
	std::cout << "Avg: pressure force: " << avgPressureForce / m_particles.size() << " bucket move: " << avgBucketMv << "\n";
#endif
}
void SimpleFluid::update(float dt, EntityId id)
{
	if (m_densities.empty())
		return;
	m_ecs->get_component<DynamicModel>(id).m_children[0].material.m_diffuse = Vector3(
		(m_densities[get_particle(id).index] - m_targetDensity) / 5.f,  // R dens > tDens
		0,																// G
		(m_targetDensity - m_densities[get_particle(id).index])			// B tDens > dens
	);
}
void SimpleFluid::gui_show_system()
{
	float energy{ 0 };
	for (auto pId : m_entities)
	{
		auto& p = get_particle(pId);
		energy += 0.5f * glm::dot(p.position - p.lastPosition, p.position - p.lastPosition);
	}
	ImGui::DragFloat("Total Kinetic Energy", &energy, 0);
}

Vector2 SimpleFluid::pressure_force(Particle& particle, Vector2 predictedPosition)
{
	Vector2 force{ 0,0 };
	auto surroundingParticles = surrounding_buckets(predictedPosition);
	for (auto pPtr : surroundingParticles)
	{
		Particle& p = *pPtr;
		if (particle.index == p.index)
			continue;

		// this -> other
		Vector2 dir = p.position - predictedPosition;
		float dist = fmaxf(0.01f, glm::length(dir));
		if (dist > m_smoothingRadius)
			continue;

		dir = dir / dist;
		float density = fmaxf(0.01f, m_densities[p.index]);
		float sharedPressure = (density_to_pressure(density) + density_to_pressure(m_densities[particle.index])) / 2.f;
		force += sharedPressure * dir * influence_slope(m_smoothingRadius, dist) / density;
		force -= m_collisionDamping * (predictedPosition - particle.position);
	}

	float density = fmaxf(0.01f, m_targetDensity * m_wallForceMultiplier);
	float sharedPressure = (density_to_pressure(density) + density_to_pressure(m_densities[particle.index])) / 2.f;

	force += sharedPressure * Vector2{ -1, 0 } * influence_slope(m_smoothingRadius, predictedPosition.x - m_minBounds.x) / density;
	force += sharedPressure * Vector2{ 1, 0 }  * influence_slope(m_smoothingRadius, m_maxBounds.x - predictedPosition.x) / density;
	force += sharedPressure * Vector2{ 0, -1 } * influence_slope(m_smoothingRadius, predictedPosition.y - m_minBounds.y) / density;
	force += sharedPressure * Vector2{ 0, 1 }  * influence_slope(m_smoothingRadius, m_maxBounds.y - predictedPosition.y) / density;

	return force;
}
float SimpleFluid::density_to_pressure(float density)
{
	// positive for dens > tDens
	// negative for tDens > dens
	return (density - m_targetDensity) * m_pressureMultiplier;
}
float SimpleFluid::wall_force(float d)
{
	if (d > m_smoothingRadius)
		return 0;

	// orig func: powf(m_smoothingRadius - d, 4.f) / (2.f * PI * powf(m_smoothingRadius, 5.f) / 5.f)
	return -20.f * powf(m_smoothingRadius - d, 3.f) / (2.f * PI * powf(m_smoothingRadius, 5.f));
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
	return -3.f * powf(rad - d, 2.f) / influence_volume(rad);
}
float SimpleFluid::influence_volume(float rad)
{
	return PI * powf(rad, 4.f) / 2.f;
}

void SimpleFluid::cache_densities()
{
	if (m_densities.size() < m_entities.size())
		m_densities.resize(m_entities.size());

	for (auto pPtr : m_particles)
	{
		m_densities[pPtr->index] = density_at(pPtr->position);
	}
}
float SimpleFluid::density_at(Vector2 position)
{
	float density{ 0 };
	auto buckets = surrounding_buckets(position);
	for (auto pPtr : buckets)
	{
		float d = glm::length(pPtr->position - position);
		density += influence(m_smoothingRadius, d);
	}
	return density;
}

void SimpleFluid::integrate(Vector2& position, Vector2& lastPosition, const Vector2& acceleration, float dt)
{
	classic_verlet(position, lastPosition, acceleration, dt);
}
void SimpleFluid::classic_verlet(Vector2& position, Vector2& lastPosition, const Vector2& acceleration, float dt)
{
	Vector2 nextPos = 2.f * position - lastPosition + 0.5f * acceleration * dt * dt;

	lastPosition = position;
	position = nextPos;
}

void SimpleFluid::create_buckets()
{
	Vector2 size = m_maxBounds - m_minBounds;
	uint32_t xCount = (uint32_t) ceilf(size.x / m_bucketSize) + 1;
	m_yBuckets = (uint32_t) ceilf(size.y / m_bucketSize) + 1;

	m_buckets.resize(xCount * m_yBuckets);
}
uint32_t SimpleFluid::pos_to_bucket_index(Vector2 pos)
{
	pos = glm::clamp(pos, m_minBounds, m_maxBounds);
	return (static_cast<uint32_t>((pos.x - m_minBounds.x) / m_bucketSize)) * m_yBuckets
		  + static_cast<uint32_t>((pos.y - m_minBounds.y) / m_bucketSize);
}
std::vector<Particle*>& SimpleFluid::bucket_at(Vector2 pos)
{
	uint32_t index = pos_to_bucket_index(pos);
	return m_buckets[pos_to_bucket_index(pos)];
}
std::vector<Particle*> SimpleFluid::surrounding_buckets(Vector2 pos)
{
	std::vector<Particle*> buckets;
	float d = m_smoothingRadius / m_bucketSize;
	for (float x = -d; x < d+1; x++)
	{
		for (float y = -d; y < d+1; y++)
		{
			auto& bucket = bucket_at(pos + Vector2{ x * m_bucketSize, y * m_bucketSize });
			buckets.insert(buckets.end(), bucket.begin(), bucket.end());
		}
	}
	return buckets;
}

Particle& SimpleFluid::get_particle(EntityId id)
{
	return m_ecs->get_component<Particle>(id);
}
bool SimpleFluid::bounds_check(Particle& particle)
{
	bool changed{ false };
	if (particle.position.x < m_minBounds.x || particle.position.x > m_maxBounds.x)
	{
		//particle.velocity.x *= -1;
		changed = true;
	}
	if (particle.position.y < m_minBounds.y || particle.position.y > m_maxBounds.y)
	{
		//particle.velocity.y *= -1;
		changed = true;
	}

	if (changed)
		particle.lastPosition = particle.position;
	particle.position = glm::clamp(particle.position, m_minBounds, m_maxBounds);

	return changed;
}