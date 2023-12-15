#include "pbd.h"

PBDParticle::PBDParticle() :
      position{ 0.f }, oldPosition{ 0.f }, velocity{ 0.f }, mass{ 1.f }, invmass{ 1.f }
{}

void PBDSystem::awake(EntityId id)
{
      auto& particle = m_ecs->get_component<PBDParticle>(id);
      particle.oldPosition = particle.position;
}

void PBDSystem::start()
{
      
}
void PBDSystem::update(float dt)
{
      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            if (particle.invmass != 0)
                  particle.invmass = 1.f / particle.mass;
            particle.velocity += dt * particle.invmass * external_force(particle.position);
      }

      damp_velocities();

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.position = particle.position + dt * particle.velocity;
      }

      generate_collision_constraints();

      solve_constraints();

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.velocity = (particle.position - particle.oldPosition) / dt;
            particle.oldPosition = particle.position;
      }

      velocity_update();

      // sync transform
      for (EntityId entity : m_entities)
            m_ecs->get_component<Transform>(entity).position = Vector3(get_particle(entity).position, 0);
}

// ---------------------------------------
// PRIVATE MEHODS
// ---------------------------------------

void PBDSystem::damp_velocities()
{

}
void PBDSystem::velocity_update()
{

}
void PBDSystem::generate_collision_constraints()
{

}
void PBDSystem::solve_constraints()
{
      solve_seidel_gauss();
}
void PBDSystem::solve_seidel_gauss()
{
      // premature optimization YAY
      std::vector<PBDParticle*> particles;
      std::vector<Vec> deltas;
      for (int i = 0; i < m_solverIterations; i++)
      {
            // project constraints
            for (Constraint* constraint : m_constraints)
            {
                  particles.clear();
                  for (auto eId : constraint->m_entities)
                        particles.push_back(&get_particle(eId));

                  float acc = 0; Vec g;
                  for (size_t j = 0; j < particles.size(); j++)
                  {
                        g = constraint->constraint_gradient(j, particles);
                        acc += particles[j]->invmass * glm::dot(g, g);
                  }
                  float constraintErr = constraint->constraint(particles);
                  if (
                        constraint->m_type == Inequality && constraintErr >= 0
                        || constraint->m_type == InverseInequality && constraintErr <= 0
                  )
                        continue;

                  float scalingFactor = constraintErr / acc;

                  deltas.clear();
                  for (size_t j = 0; j < particles.size(); j++)
                  {
                        deltas.push_back(
                              -scalingFactor
                              * particles[j]->invmass
                              * constraint->constraint_gradient(j, particles)
                        );
                  }
                  float correctedStiffness = 1 - std::pow(1 - constraint->m_stiffness, constraint->m_cardinality);
                  for (size_t j = 0; j < particles.size(); j++)
                  {
                        particles[j]->position += correctedStiffness * deltas[j];
                  }
            }
      }
}

PBDParticle& PBDSystem::get_particle(EntityId id)
{
      return m_ecs->get_component<PBDParticle>(id);
}
Vec PBDSystem::external_force(Vec pos)
{
      return Vector2(0, 9.81f);
}

// ---------------------------------------
// COLLISION CONSTRAINT
// ---------------------------------------

Constraint::Constraint(Cardinality cardinality, std::vector<EntityId> entities) :
      m_cardinality { cardinality }, m_entities{ entities }, m_stiffness{ 1.f }, m_type{ Equality }
{}

// ---------------------------------------
// COLLISION CONSTRAINT
// ---------------------------------------

CollisionConstraint::CollisionConstraint(float distance, std::vector<EntityId> entities) :
      Constraint(2, entities), m_distance{ distance }
{}
float CollisionConstraint::constraint(InParticles particles)
{
      return glm::length(particles[0]->position - particles[1]->position) - m_distance;
}
Vec CollisionConstraint::constraint_gradient(size_t der, InParticles particles)
{
      Vec n = glm::normalize(particles[0]->position - particles[1]->position);
      return n * (static_cast<float>(der) * -2.f + 1.f);
}
