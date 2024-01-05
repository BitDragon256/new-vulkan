#include "pbd/fluid_constraints.h"

// ---------------------------------
// KERNEL FUNCTIONS
// ---------------------------------



// ---------------------------------
// PUBLIC METHODS
// ---------------------------------

SPHConstraint::SPHConstraint(std::vector<EntityId> entities) :
      Constraint(0, entities)
{}

float SPHConstraint::constraint(InParticles particles)
{
      return kernel(glm::length(particles[0]->position - particles[1]->position));
}
Vec SPHConstraint::constraint_gradient(size_t der, InParticles particles)
{
      return Vec(0);
}

// ---------------------------------
// PRIVATE METHODS
// ---------------------------------

float SPHConstraint::kernel(float distance)
{
      return 0;
}
Vec SPHConstraint::kernel_gradient(float distance)
{
      return Vec(0);
}
