#pragma once

#include "pbd.h"

inline float KernelMultiplier = 1.f;
inline float TargetPressure = 2.5f;
inline float PressureMultiplier = .001f;
inline float KernelRadius = 1.f;

class SPHConstraint : public Constraint
{
public:
      SPHConstraint(std::vector<EntityId> entities);

      float constraint(InParticles particles) override;
      Vec constraint_gradient(size_t der, InParticles particles) override;

private:

};

class SPHConstraintGenerator : public ConstraintGenerator
{
public:
      std::vector<Constraint*> create(
            EntityId particle, std::vector<EntityId> surrounding,
            ECSManager* ecs
      ) override;
};