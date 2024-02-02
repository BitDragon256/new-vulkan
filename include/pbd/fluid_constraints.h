#pragma once

#include "pbd.h"

inline float KernelMultiplier = 8.f / PI;
inline float KernelGradientMultiplier = 48.f / PI;
inline float TargetPressure = 2.5f;
inline float PressureMultiplier = .001f;
inline float KernelRadius = 1.f;
inline float ParticleRadius = .25f;
inline float BaseDensity = 1000.f;

inline size_t KernelFunctionIndex = 1;
inline size_t KernelGradientFunctionIndex = 1;
inline const char* KernelFunctionNames[] = { "Cubic Spline", "Cubic", "Spiky" };

class SPHConstraint : public Constraint
{
public:
      SPHConstraint(std::vector<EntityId> entities, ECSManager* ecs);

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

inline float AttractionDistance = 1.4f;
inline float AttractionCompliance = 3.f;
inline float DetentionDistance = 1.f;
inline float DetentionCompliance = 4.f;

class AttractionFluidConstraintGenerator : public ConstraintGenerator
{
public:
      std::vector<Constraint*> create(
            EntityId particle, std::vector<EntityId> surrounding,
            ECSManager* ecs
      ) override;
};
