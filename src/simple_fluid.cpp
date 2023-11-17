#include "simple_fluid.h"

#include <math.h>
#include <time.h>

#include "logger.h"

#define SF_PROFILER
#undef SF_PROFILER

#ifdef SF_PROFILER
#define PROFILE_START(X) m_profiler.start_measure(X);
#define PROFILE_END(X) m_profiler.end_measure(X, true);
#else
#define PROFILE_START(X)
#define PROFILE_END(X) 0.f
#endif

#define SIMPLE_FLUID_THREADING
//#undef SIMPLE_FLUID_THREADING

SimpleFluid::SimpleFluid() : m_pIndex{ 0 }
{
	create_buckets();
	m_threadPool.initialize(m_threadCount);
}
void SimpleFluid::awake(EntityId id)
{
	Particle& particle = get_particle(id);
	m_particles.emplace_back(&particle);
	bucket_at(particle.position).emplace_back(&particle);

	particle.lastPosition = particle.position;

	particle.index = m_pIndex;
	m_pIndex++;
}
void SimpleFluid::update(float dt)
{
	if (!m_active)
		return;

	m_profiler.begin_label("simple_fluid_update");

	float totalTime = 0;
	PROFILE_START("cache densities");
	cache_densities();
	totalTime += PROFILE_END("cache densities", true);

	// Threading force calculations
	PROFILE_START("calc forces");

#ifdef SIMPLE_FLUID_THREADING
	for (size_t i = 0; i < m_buckets.size(); i += m_threadBucketDiff)
	{
		m_threadPool.doJob(std::bind(&SimpleFluid::calc_forces, this, i, i + m_threadBucketDiff, dt));
	}
	//m_threadPool.wait_for_finish();
#else
	calc_forces(0, m_buckets.size(), dt);
#endif
	totalTime += PROFILE_END("calc forces", true);
	PROFILE_START("assign buckets");

	size_t i = 0;
	for (auto particlePtr : m_particles)
	{
		auto& bucket = bucket_at(particlePtr->position);
		bucket.erase(std::find(bucket.begin(), bucket.end(), particlePtr));

		integrate(particlePtr->position, particlePtr->lastPosition, particlePtr->velocity, particlePtr->acc, dt);

		if (bounds_check(*particlePtr));
		//	particle.position += particle.velocity * dt;

		bucket_at(particlePtr->position).push_back(particlePtr);

		// sync pos with transform
		m_ecs->get_component<Transform>(m_entities[i++]).position = { particlePtr->position.x, particlePtr->position.y, 0 };
	}

	totalTime += PROFILE_END("assign buckets", true);
	//m_profiler.out_buf() << "total time measured " << totalTime << " seconds\n\n";
	m_profiler.end_label();
}
void SimpleFluid::calc_forces(size_t start, size_t end, float dt)
{
	// for (size_t i = start; i < end && i < m_particles.size(); i++)
	auto bucketIt = m_bucketLockMutices.begin();
	for (int i = 0; i < start; i++) bucketIt++;
	for (size_t i = start; i < end && i < m_buckets.size(); i++, bucketIt++)
	{
#ifdef SIMPLE_FLUID_THREADING
		std::lock_guard<std::mutex> lock(*bucketIt);
#endif
		for (auto particlePtr : m_buckets[i])
		{
			Particle& particle = *particlePtr;
			particle.acc = Vector2(0);

			Vector2 predictedPos = particle.position + particle.velocity * dt;
			particle.acc += Vector2 { m_gravity, 0 };

			particle.acc += mouse_force(particle.position, particle.velocity);

			//particle.velocity += pressure_force(particle) * dt;
			particle.acc += pressure_force(particle, predictedPos);
		}
	}
}
/*
void SimpleFluid::update(float dt, EntityId id)
{
	if (m_densities.empty())
		return;
	auto& p = get_particle(id);
	m_ecs->get_component<DynamicModel>(id).m_children[0].material.m_diffuse = Vector3(
		density_to_pressure(density_at(p.position)) / 20.f / m_pressureMultiplier,
		-density_to_pressure(density_at(p.position)) / 20.f / m_pressureMultiplier,
		0.f
	);
}
*/
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
		force += sharedPressure * dir * influence_grad(m_smoothingRadius, dist) / density;
		particle.velocity = m_collisionDamping * particle.velocity + (1.f - m_collisionDamping) * pPtr->velocity;
	}

	return force;
}
Vector2 SimpleFluid::mouse_force(Vector2 pos, Vector2 vel)
{
	Vector2 offset = m_mousePos - pos;
	float dst = glm::dot(offset, offset);
	if (dst < m_mouseRadius * m_mouseRadius)
	{
		dst = sqrtf(dst);
		Vector2 n = dst <= 0.01f ? Vector2(0) : offset / dst;
		float interpolation = 1.f - dst / m_mouseRadius;
		return (n * m_mouseStrength - vel) * interpolation;
	}
	return Vector2(0);
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
	return powf((rad - d) * m_influenceInner, m_influencePower) / influence_volume(rad);
}
float SimpleFluid::influence_grad(float rad, float d)
{
	if (d > rad)
		return 0;
	return -m_influenceInner * m_influencePower * powf(rad - d, m_influencePower - 1.f);// / influence_volume(rad);
}
float SimpleFluid::influence_volume(float rad)
{
	return PI * powf(rad, m_influencePower + 1.f) / (m_influencePower + 1.f) * m_influenceInner;
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

void SimpleFluid::integrate(Vector2& position, Vector2& lastPosition, Vector2& velocity, const Vector2& acceleration, float dt)
{
	// classic_verlet(position, lastPosition, velocity, acceleration, dt);
	implicit_euler(position, velocity, acceleration, dt);
}
void SimpleFluid::classic_verlet(Vector2& position, Vector2& lastPosition, Vector2& velocity, const Vector2& acceleration, float dt)
{
	Vector2 nextPos = 2.f * position - lastPosition + 0.5f * acceleration * dt * dt;

	lastPosition = position;
	position = nextPos;

	velocity = (lastPosition - position) / dt;
}
void SimpleFluid::implicit_euler(Vector2& position, Vector2& velocity, const Vector2& acceleration, float dt)
{
	velocity += acceleration * dt;
	position += velocity * dt;
}

void SimpleFluid::create_buckets()
{
	Vector2 size = m_maxBounds - m_minBounds;
	uint32_t xCount = (uint32_t) ceilf(size.x / m_bucketSize) + 1;
	m_yBuckets = (uint32_t) ceilf(size.y / m_bucketSize) + 1;
	size_t bucketCount = xCount * m_yBuckets;

	m_buckets.resize(bucketCount);
	m_bucketLockMutices.resize(bucketCount);
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
			// buckets.insert(buckets.end(), bucket.begin(), bucket.end());
			for (auto& el : bucket)
				buckets.push_back(el);
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
		particle.velocity.x *= -1;
		changed = true;
	}
	if (particle.position.y < m_minBounds.y || particle.position.y > m_maxBounds.y)
	{
		particle.velocity.y *= -1;
		changed = true;
	}

	if (changed)
		particle.lastPosition = particle.position;
	particle.position = glm::clamp(particle.position, m_minBounds, m_maxBounds);

	return changed;
}