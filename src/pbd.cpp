#include "pbd.h"

#include <numeric>

// #include <linalg/gsl_linalg.h>

#include "render.h"

PBDParticle::PBDParticle() :
      position{ 0.f }, oldPosition{ 0.f }, tempPosition{ 0.f }, velocity{ 0.f }, mass{ 1.f }, invmass{ 1.f }, radius{ 1.f }
{}

PBDSystem::PBDSystem() :
      m_grid{ PBD_GRID_SIZE }, m_constraintStart{ 0 }
{}

void PBDSystem::awake(EntityId id)
{
      auto& particle = m_ecs->get_component<PBDParticle>(id);
      particle.oldPosition = particle.position;
      m_grid.insert_particle(particle.position, id);
}

void PBDSystem::start()
{
      
}
void PBDSystem::update(float dt)
{
      xpbd_update(dt);

      sync_transform();
}
void PBDSystem::pbd_update(float dt)
{
      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.scale = m_ecs->get_component<Transform>(entity).scale;

            if (particle.invmass != 0)
                  particle.invmass = 1.f / particle.mass;
            particle.velocity += dt * particle.invmass * external_force(particle.position);

            particle.tempPosition = particle.position;
            particle.oldPosition = particle.position;
      }

      damp_velocities();

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.position = particle.position + dt * particle.velocity;

            sync_grid(particle, entity);
      }

      m_profiler.start_measure("gen col const");
      generate_constraints();
      logger::log("gen col const", m_profiler.end_measure("gen col const"));

      m_profiler.start_measure("solve const");
      solve_constraints();
      logger::log("solve const: ", m_profiler.end_measure("solve const"));

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.velocity = (particle.position - particle.oldPosition) / dt;
      
            sync_grid(particle, entity);
      }

      velocity_update();

      // draw_debug_lines();
}
void PBDSystem::xpbd_update(float dt)
{
      float sdt = dt / static_cast<float>(m_substeps);

      m_profiler.start_measure("gen const");
      generate_constraints();
      logger::log("generate constraints", m_profiler.end_measure("gen const"));

      m_profiler.start_measure("substeps");

      for (int substep = 0; substep < m_substeps; substep++)
      {
            xpbd_substep(sdt);
      }

      logger::log("substeps", m_profiler.end_measure("substeps"));
}
void PBDSystem::xpbd_substep(float dt)
{
      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.scale = m_ecs->get_component<Transform>(entity).scale;

            if (particle.invmass != 0)
                  particle.invmass = 1.f / particle.mass;
            particle.velocity += dt * particle.invmass * external_force(particle.position);

            particle.tempPosition = particle.position;
            particle.oldPosition = particle.position;
      }

      damp_velocities();

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.position = particle.position + dt * particle.velocity;

            sync_grid(particle, entity);
      }

      xpbd_solve(dt);

      for (EntityId entity : m_entities)
      {
            auto& particle = get_particle(entity);
            particle.velocity = (particle.position - particle.oldPosition) / dt;
      
            sync_grid(particle, entity);
      }

      velocity_update();
}

void PBDSystem::register_self_generating_constraint(ConstraintGenerator* generator)
{
      m_constraintGenerators.emplace_back(generator);
}

// ---------------------------------------
// PRIVATE MEHODS
// ---------------------------------------


void PBDSystem::damp_velocities()
{
      for (auto e : m_entities)
      {
            auto& p = get_particle(e);
            p.velocity *= m_dampingConstant;
      }

//      float massSum = 0.f; for (auto e : m_entities) massSum += get_particle(e).mass;
//
//      Vec xcm{ 0.f };
//      for (auto e : m_entities)
//      {
//           const auto& p = get_particle(e);
//           if (p.invmass == 0.f)
//           {
//                 xcm = Vec(p.position);
//                 massSum = 1.f;
//                 break;
//           }
//           xcm += p.position * p.mass;
//      }
//      xcm /= massSum;
//
//      Vec vcm{ 0.f };
//      for (auto e : m_entities)
//      {
//           const auto& p = get_particle(e);
//           if (p.invmass == 0.f)
//           {
//                 vcm = Vec(p.velocity);
//                 massSum = 1.f;
//                 break;
//           }
//           vcm += p.velocity * p.mass;
//      }
//      vcm /= massSum;
//
//      // Rotational Impulse
//      Vec L{ 0.f };
//      for (auto e : m_entities)
//      {
//            const auto& p = get_particle(e);
//            L += glm::cross(p.position - xcm, p.mass * p.velocity);
//      }
//
//#ifdef PBD_3D
//      glm::mat3x3 I{ 0.f };
//#else
//      glm::mat2x2 I{ 0.f }
//#endif
//      for (auto e : m_entities)
//      {
//            const auto& p = get_particle(e);
//            Vec r = p.position - xcm;
//#ifdef PBD_3D
//            glm::mat3x3 rs {
//                  0, r[2], -r[1],
//                  -r[2], 0, r[0],
//                  r[1], -r[0], 0
//            };
//            I += rs * glm::transpose(rs) * p.mass;
//#endif
//      }
//
//      Vec omega = glm::inverse(I) * L;
//      for (auto e : m_entities)
//      {
//            auto& p = get_particle(e);
//            Vec dv = vcm + glm::cross(omega, p.position - xcm) - p.velocity;
//            if (p.invmass != 0.f)
//                  p.velocity += m_dampingConstant * dv;
//      }
}
void PBDSystem::velocity_update()
{

}
void PBDSystem::generate_constraints()
{
      // clear collision constraints
      m_constraints.erase(m_constraints.begin() + m_constraintStart, m_constraints.end());

      m_constraintStart = m_constraints.size();

      float avgNeighbors{ 0.f };

      std::vector<EntityId> unfilteredSurroundingParticles;
      std::vector<EntityId> surroundingParticles;
      std::sort(m_entities.begin(), m_entities.end());
      #pragma omp parallel default(shared)
      {
            #pragma omp for schedule(static)
            for (EntityId entity : m_entities)
            {
                  const auto& particle = get_particle(entity);
                  if (particle.radius == 0)
                        continue;

                  surroundingParticles.clear();
                  unfilteredSurroundingParticles.clear();

                  m_grid.surrounding_particles(particle.position, unfilteredSurroundingParticles);
                  std::copy_if(
                        unfilteredSurroundingParticles.begin(),
                        unfilteredSurroundingParticles.end(),
                        std::back_inserter(surroundingParticles),
                        [&](EntityId e) {
                              Vec d = get_particle(e).position - particle.position;
                              return glm::dot(d, d) <= PBD_GRID_SIZE * PBD_GRID_SIZE;
                        }
                  );

                  avgNeighbors += static_cast<float>(surroundingParticles.size());
                  //for (auto e : surroundingParticles)
                  //      m_ecs->m_renderer->gizmos_draw_line(get_particle(entity).position, get_particle(e).position, Color(1.f), 0.05f);

                  for (const auto constraintGenerator : m_constraintGenerators)
                  {
                        auto constraints = constraintGenerator->create(entity, surroundingParticles, m_ecs);
                        m_constraints.insert(m_constraints.end(), constraints.cbegin(), constraints.cend());
                  }
            }
      }

      avgNeighbors /= static_cast<float>(m_entities.size());
      logger::log("average neighbors", avgNeighbors);
}
void PBDSystem::solve_constraints()
{
      solve_seidel_gauss();
      // solve_sys();
}
void PBDSystem::solve_seidel_gauss()
{
      // TODO get from git history
}
void PBDSystem::xpbd_solve(float dt)
{
      std::vector<Vec> deltas;
      std::vector<bool> isConstraint;

      for (int i = 0; i < m_solverIterations; i++)
      {
            isConstraint.resize(m_constraints.size());
            // project constraints
            //#pragma omp parallel default(shared)
            {
                  //#pragma omp for schedule(static)
                  for (size_t constraintIndex = 0; constraintIndex < m_constraints.size(); constraintIndex++)
                  {
                        Constraint* constraint = m_constraints[constraintIndex];

                        isConstraint[constraintIndex] = false;

                        constraint->m_gradients.clear();
                        constraint->m_gradients.reserve(constraint->m_entities.size());
                        for (size_t j = 0; j < constraint->m_particles.size(); j++)
                              constraint->m_gradients.push_back(constraint->constraint_gradient(j, constraint->m_particles));

                        float constraintErr = constraint->constraint(constraint->m_particles);

                        if (
                              constraint->m_type == Inequality && constraintErr >= 0
                              || constraint->m_type == InverseInequality && constraintErr <= 0
                              )
                              continue;

                        // calculate the overall constraint gradient

                        float sqrGradientSum = 0;
                        Vec gradientSum{ 0.f };
                        for (size_t j = 0; j < constraint->m_particles.size(); j++)
                        {
                              gradientSum += constraint->m_gradients[j];
                              sqrGradientSum +=
                                    constraint->m_particles[j]->invmass
                                    * glm::dot(constraint->m_gradients[j], constraint->m_gradients[j]);
                        }

                        sqrGradientSum += glm::dot(gradientSum, gradientSum);
                        sqrGradientSum += constraint->m_compliance / std::powf(dt, 2.f);

                        if (sqrGradientSum == 0.f)
                              continue;

                        float scalingFactor = constraintErr / (sqrGradientSum + .000001f);
                        if (scalingFactor != scalingFactor)
                              continue;

                        constraint->m_scalingFactor = scalingFactor;
                        isConstraint[constraintIndex] = true;
                  }
            }

            //#pragma omp parallel default(shared)
            {
                  //#pragma omp for schedule(static)
                  for (size_t constraintIndex = 0; constraintIndex < m_constraints.size(); constraintIndex++)
                  {
                        if (!isConstraint[constraintIndex])
                              continue;
                        Constraint* constraint = m_constraints[constraintIndex];

                        // calculate delta
                        Vec delta{ 0.f };
                        for (size_t j = 0; j < constraint->m_particles.size(); j++)
                        {
                              delta +=
                                    -(constraint->m_scalingFactor) // +particles[j]->scalingFactor)
                                    * constraint->m_particles[j]->invmass
                                    * constraint->m_gradients[j];
                        }
                        
                        constraint->m_particles.front()->position += delta;
                  }
            }
      }
}
void PBDSystem::solve_sys()
{

}

PBDParticle& PBDSystem::get_particle(EntityId id)
{
      return m_ecs->get_component<PBDParticle>(id);
}
Vec PBDSystem::external_force(Vec pos)
{
#ifdef PBD_3D
      return Vector3(0.f, 0.f, -9.81f);
#else
      return Vector2(0, 9.81f);
#endif
}
void PBDSystem::sync_grid(PBDParticle& particle, EntityId entity)
{
      m_grid.change_particle(particle.tempPosition, particle.position, entity);
      particle.tempPosition = particle.position;
}
void PBDSystem::sync_transform()
{
      for (EntityId entity : m_entities)
      {
#ifdef PBD_3D
            m_ecs->get_component<Transform>(entity).position = get_particle(entity).position;
            if (entity != 0)
                  m_ecs->get_component<DynamicModel>(entity).m_children.front().material->m_diffuse
                  = Color(
                        0.f, 0.f,
                        glm::length(get_particle(entity).velocity) / 25.f
                        );
#else
            m_ecs->get_component<Transform>(entity).position = Vector3(get_particle(entity).position, 0);
#endif
      }
}

void PBDSystem::draw_debug_lines()
{
      // debug lines
      std::vector<EntityId> surroundingParticles;
      for (EntityId entity : m_entities)
      {
            const auto& particle = get_particle(entity);
            if (particle.radius == 0)
                  continue;
            surroundingParticles.clear();
            m_grid.surrounding_particles(particle.position, surroundingParticles);
            for (const auto e : surroundingParticles)
            {
                  const auto& ep = m_ecs->get_component<PBDParticle>(e);
#ifdef PBD_3D
                  m_ecs->m_renderer->gizmos_draw_line(ep.position, particle.position, Color(1.f), .1f);
#else
                  m_ecs->m_renderer->gizmos_draw_line(vec23(ep.position), vec23(particle.position), Color(1.f), .1f);
#endif
            }
      }
}

// ---------------------------------------
// CONSTRAINT
// ---------------------------------------

Constraint::Constraint(Cardinality cardinality, std::vector<EntityId> entities, ECSManager* ecs) :
      m_cardinality { cardinality }, m_entities{ entities }, m_compliance{ 0.f }, m_type{ Equality }
{
      for (size_t j = 0; j < entities.size(); j++)
            m_particles.push_back(&ecs->get_component<PBDParticle>(entities[j]));
}

// ---------------------------------------
// COLLISION CONSTRAINT
// ---------------------------------------

CollisionConstraint::CollisionConstraint(float distance, std::vector<EntityId> entities, ECSManager* ecs) :
      Constraint(2, entities, ecs), m_distance{ distance }
{}

float BoxEdgeRadius = 0.2f;
float CollisionConstraint::constraint(InParticles particles)
{
      // box constraint
      Vec d = particles[0]->position - particles[1]->position;
#ifdef PBD_3D
      d = Vec(std::fabsf(d.x), std::fabsf(d.y), std::fabsf(d.z));
      return glm::length(Vec(
            std::fmaxf(d.x - (particles[1]->scale.x + particles[0]->scale.x) / 2.f + BoxEdgeRadius, 0.f),
            std::fmaxf(d.y - (particles[1]->scale.y + particles[0]->scale.y) / 2.f + BoxEdgeRadius, 0.f),
            std::fmaxf(d.z - (particles[1]->scale.z + particles[0]->scale.z) / 2.f + BoxEdgeRadius, 0.f)
      )) - BoxEdgeRadius;
#else
      d = Vec(std::fabsf(d.x), std::fabsf(d.y));
      return glm::length(Vec(
            std::fmaxf(d.x, particles[0]->scale.x),
            std::fmaxf(d.y, particles[0]->scale.y)
      ));
#endif

      // sphere constraint
      // return glm::length(particles[0]->position - particles[1]->position) - m_distance;
}
Vec CollisionConstraint::constraint_gradient(size_t der, InParticles particles)
{
      // box constraint
      Vec d = particles[0]->position - particles[1]->position;
#ifdef PBD_3D
      Vec ud = Vec(std::fabsf(d.x), std::fabsf(d.y), std::fabsf(d.z));
      Vec n = Vec(
            std::fmaxf(ud.x - (particles[1]->scale.x + particles[0]->scale.x) / 2.f, 0.f) * d.x / ud.x,
            std::fmaxf(ud.y - (particles[1]->scale.y + particles[0]->scale.y) / 2.f, 0.f) * d.y / ud.y,
            std::fmaxf(ud.z - (particles[1]->scale.z + particles[0]->scale.z) / 2.f, 0.f) * d.z / ud.z
      );
#else
      d = Vec(std::fabsf(d.x), std::fabsf(d.y));
      Vec n = Vec(
            std::fmaxf(d.x, particles[0]->scale.x),
            std::fmaxf(d.y, particles[0]->scale.y)
      ));
#endif
      float l = glm::length(n) - 0.2f;
      if (l != 0.f)
            n /= l;
      return n * (static_cast<float>(der) * -2.f + 1.f);

      // sphere collision
      //Vec d = particles[0]->position - particles[1]->position;
      //float length = glm::length(d);
      //if (length != 0)
      //      d /= length;
      //return d * (static_cast<float>(der) * -2.f + 1.f);
}

std::vector<Constraint*> CollisionConstraintGenerator::create(
      EntityId particle, std::vector<EntityId> surrounding,
      ECSManager* ecs
)
{
      std::vector<Constraint*> constraints;
      const auto& pbdParticle = ecs->get_component<PBDParticle>(particle);

      #pragma omp parallel default(shared)
      {
            #pragma omp for schedule(static)
            for (const auto& surroundingParticle : surrounding)
            {
                  if (particle >= surroundingParticle)
                        continue;

                  const auto& other = ecs->get_component<PBDParticle>(surroundingParticle);
                  if (other.radius == 0)
                        continue;

                  auto constraint = new CollisionConstraint(
                        pbdParticle.radius + other.radius,
                        { particle, surroundingParticle },
                        ecs
                  );
                  constraint->m_compliance = 0.f;
                  constraint->m_type = Inequality;

                  constraints.emplace_back(constraint);
            }
      }

      return constraints;
}
