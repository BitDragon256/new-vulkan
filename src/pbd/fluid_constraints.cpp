#include "pbd/fluid_constraints.h"

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

float cubic_spline_gradient(float q, float h)
{
      float sigma = 1.f / (PI * std::powf(h, 3.f));
      float W;
      if (q <= 1)
            W = sigma * (9.f / 4.f * std::pow(q, 2.f) - 3.f * q);
      else if (q <= 2)
            W = sigma * 3.f / 4.f * std::powf(2.f - q, 2.f);
      else
            W = 0;
      return W;
}

float cubic_kernel(float l, float h)
{
      float res = 0.0;
      const float q = l / h;
      if (q <= 1.0)
      {
            if (q <= 0.5)
            {
                  const float q2 = q * q;
                  const float q3 = q2 * q;
                  res = KernelMultiplier * (6.f * q3 - 6.f * q2 + 1.f);
            }
            else
            {
                  res = KernelMultiplier * (2.f * std::pow(1.f - q, 3.f));
            }
      }
      return res;
}
Vec cubic_kernel_gradient(Vec d, float h)
{
      Vec res{ 0 };
      const float rl = glm::length(d);
      const float q = rl / h;
      if (q <= 1.0)
      {
            if (rl > 1.0e-6)
            {
                  const Vec gradq = d * (1.f / (rl * h));
                  if (q <= 0.5)
                  {
                        res = KernelMultiplier * q * (3.f * q - 2.f) * gradq;
                  }
                  else
                  {
                        const float factor = 1.f - q;
                        res = KernelMultiplier * (-factor * factor) * gradq;
                  }
            }
      }

      return res;
}

// ---------------------------------
// FINAL KERNEL
// ---------------------------------

float kernel(float distance)
{
      return KernelMultiplier * cubic_kernel(distance, KernelRadius);
}
Vec kernel_gradient(Vec d)
{
      return KernelMultiplier * cubic_kernel_gradient(d, KernelRadius);
}


float density_to_pressure(float density)
{
      return (density - TargetPressure) * PressureMultiplier;
}

// ---------------------------------
// PUBLIC METHODS
// ---------------------------------

SPHConstraint::SPHConstraint(std::vector<EntityId> entities) :
      Constraint(entities.size(), entities)
{}

// returns the pressure onto particles.front()
float SPHConstraint::constraint(InParticles particles)
{
      float density = particles.front()->density;
      return density_to_pressure(density);
}
// returns the influence gradient
Vec SPHConstraint::constraint_gradient(size_t der, InParticles particles)
{
      if (der != 0)
            return Vec(0.f);
      Vec delta{ 0 };
      for (auto p = particles.begin() + 1; p != particles.end(); p++)
      {
            delta +=
                  kernel_gradient((*p)->position - particles.front()->position)
                  / particles.front()->density;
      }
      return delta;
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
      auto& pbdParticle = ecs->get_component<PBDParticle>(particle);

      pbdParticle.density = 0;
      #pragma omp parallel default(shared)
      {
            #pragma omp for schedule(static)
            for (auto s : surrounding)
            {
                  if (s == particle)
                        continue;
                  pbdParticle.density += kernel(
                        glm::length(ecs->get_component<PBDParticle>(s).position - pbdParticle.position)
                  );
            }
      }

      surrounding.insert(surrounding.begin(), particle);
      auto constraint = new SPHConstraint(
            surrounding
      );
      constraint->m_stiffness = 1.f;
      constraint->m_type = Equality;

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
