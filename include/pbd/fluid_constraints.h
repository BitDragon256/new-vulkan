#pragma once

#include "pbd.h"

class SPHConstraint : public Constraint
{
      SPHConstraint(std::vector<EntityId> entities);

      float constraint(InParticles particles) override;
      Vec constraint_gradient(size_t der, InParticles particles) override;

private:
      float kernel(float distance);
      Vec kernel_gradient(float distance);

      float m_smoothingLength;
};