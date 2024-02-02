#include "pbd/fluid_constraints.h"

#include <time.h>
#include <stdlib.h>

// ---------------------------------
// KERNEL FUNCTIONS
// ---------------------------------

float cubic_spline(float q, float h)
{
      float sigma = 1.f / (PI * std::powf(h, 3.f));
      float W;
      if (q <= 1)
            W = sigma * (
                  1.f - 3.f / 2.f * std::powf(q, 2.f) * (1.f - q / 2.f)
                  );
      else if (q <= 2)
            W = sigma / 4.f * std::powf(2.f - q, 3.f);
      else
            W = 0;
      return W;
}

Vec cubic_spline_gradient(Vec d, float h)
{
      float q = glm::length(d);
      float sigma = 1.f / (PI * std::powf(h, 3.f));
      float W;
      if (q <= 1)
            W = sigma * (9.f / 4.f * std::pow(q, 2.f) - 3.f * q);
      else if (q <= 2)
            W = sigma * 3.f / 4.f * std::powf(2.f - q, 2.f);
      else
            W = 0;
      return W * d / q;
}

float cubic_kernel(float l, float h)
{
      float res = 0.f;
      const float q = l / h;
      if (q <= 1.0)
      {
            if (q <= 0.5)
            {
                  const float q2 = q * q;
                  const float q3 = q2 * q;
                  res = (6.f * q3 - 6.f * q2 + 1.f);
            }
            else
            {
                  res = (2.f * std::pow(1.f - q, 3.f));
            }
      }
      return res;
}
Vec cubic_kernel_gradient(Vec d, float h)
{
      Vec res{ 0.f };
      const float rl = glm::length(d);
      const float q = rl / h;
      if (q <= 1.0)
      {
            if (rl > 1.0e-6)
            {
                  const Vec gradq = d / (rl * h);
                  if (q <= 0.5)
                  {
                        res = q * (3.f * q - 2.f) * gradq;
                  }
                  else
                  {
                        const float factor = 1.f - q;
                        res = (-factor * factor) * gradq;
                  }
            }
            else
            {
                  res = Vec(
                        (rand() % 1000) / 1000.f - .5f,
                        (rand() % 1000) / 1000.f - .5f,
                        (rand() % 1000) / 1000.f - .5f
                  );
            }
      }

      return res;
}

float spiky_kernel(float l, float h)
{
      float res = 0.f;
      const float q = l / h;
      if (q <= 1.0)
      {
            res = (2.f * std::pow(1.f - q, 3.f));
      }
      return res;
}
Vec spiky_kernel_gradient(Vec d, float h)
{
      Vec res{ 0.f };
      const float rl = glm::length(d);
      const float q = rl / h;
      if (q <= 1.0)
      {
            if (rl > 1.0e-6)
            {
                  const Vec gradq = d / (rl * h);
                  const float factor = 1.f - q;
                  res = (-factor * factor) * gradq;
            }
            else
            {
                  res = Vec(
                        (rand() % 1000) / 1000.f - .5f,
                        (rand() % 1000) / 1000.f - .5f,
                        (rand() % 1000) / 1000.f - .5f
                  );
            }
      }

      return res;
}
// ---------------------------------
// FINAL KERNEL
// ---------------------------------

float kernel(float distance)
{
      float res = 0.f;
      switch (KernelFunctionIndex)
      {
      case 0:
            res = cubic_spline(distance, KernelRadius);
            break;

      case 1:
            res = cubic_kernel(distance, KernelRadius);
            break;

      case 2:
            res = spiky_kernel(distance, KernelRadius);
            break;

      default:
            break;
      }
      return KernelMultiplier * res;
}
Vec kernel_gradient(Vec d)
{
      Vec res = Vec(0.f);
      switch (KernelFunctionIndex)
      {
      case 0:
            res = cubic_spline_gradient(d, KernelRadius);
            break;

      case 1:
            res = cubic_kernel_gradient(d, KernelRadius);
            break;

      case 2:
            res = spiky_kernel_gradient(d, KernelRadius);
            break;

      default:
            break;
      }
      return KernelGradientMultiplier * res;
}


float density_to_pressure(float density)
{
      return (density - TargetPressure) * PressureMultiplier;
}

// ---------------------------------
// PUBLIC METHODS
// ---------------------------------

SPHConstraint::SPHConstraint(std::vector<EntityId> entities, ECSManager* ecs) :
      Constraint(entities.size(), entities, ecs)
{
      srand(time(0));
      m_compliance = 0.f;
      m_type = Equality;
}

// returns the pressure onto particles.front()
float SPHConstraint::constraint(InParticles particles)
{
      // return density_to_pressure(particles.front()->density);
      // return std::max(particles.front()->density / BaseDensity - 1.f, 0.f);

      return density_to_pressure(particles.front()->density / BaseDensity);
}
// returns the influence gradient
Vec SPHConstraint::constraint_gradient(size_t der, InParticles particles)
{
      if (der == 0)
            return Vec(0.f);
      return
            (density_to_pressure(particles.front()->density) + density_to_pressure(particles[der]->density)) *
            //density_to_pressure(particles.front()->density) *
            kernel_gradient(particles[der]->position - particles.front()->position)
            / BaseDensity
            * particles[der]->fluidMass;
}

// ---------------------------------
// CONSTRAINT GENERATOR
// ---------------------------------
std::vector<Constraint*> SPHConstraintGenerator::create(
      EntityId particle, std::vector<EntityId> surrounding,
      ECSManager* ecs
)
{
      std::vector<Constraint*> constraints;

      if (surrounding.size() <= 1)
            return constraints;

      auto& pbdParticle = ecs->get_component<PBDParticle>(particle);

      pbdParticle.fluidMass = 0.8f * std::powf(2.f * ParticleRadius, 3.f) * BaseDensity;
      pbdParticle.density = pbdParticle.fluidMass * kernel(0);
      #pragma omp parallel default(shared)
      {
            #pragma omp for schedule(static)
            for (auto s : surrounding)
            {
                  if (s == particle)
                        continue;
                  pbdParticle.density
                        += pbdParticle.fluidMass
                        * kernel(
                              glm::length(
                                    ecs->get_component<PBDParticle>(s).position
                                    - pbdParticle.position
                              )
                        );
            }
      }

      surrounding.insert(surrounding.begin(), particle);
      auto constraint = new SPHConstraint(
            surrounding, ecs
      );

      constraints.emplace_back(constraint);

      //for (const auto& surroundingParticle : surrounding)
      //{
      //      if (particle >= surroundingParticle)
      //            continue;

      //      const auto& other = ecs->get_component<PBDParticle>(surroundingParticle);

      //      auto constraint = new SPHConstraint(
      //            { particle, surroundingParticle }
      //      );
      //      constraint->m_stiffness = 1.f;
      //      constraint->m_type = Equality;

      //      constraints.emplace_back(constraint);
      //}

      return constraints;
}
