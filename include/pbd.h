#pragma once

#include <array>
#include <vector>

#include "ecs.h"
#include "model-handler.h"

#include "nve_types.h"

typedef float Real;
typedef size_t Cardinality;

typedef Vector2 Vec;

struct PBDParticle
{
      PBDParticle();
      Vec oldPosition;
      Vec position;
      Vec velocity;
      float mass;
      float invmass;
};

enum ConstraintType { Equality, Inequality, InverseInequality };

typedef std::vector<PBDParticle*> InParticles;
class Constraint
{
public:
      Constraint(Cardinality cardinality, std::vector<EntityId> entities);

      const Cardinality m_cardinality;
      float m_stiffness;
      std::vector<EntityId> m_entities;
      ConstraintType m_type;
      virtual float constraint(InParticles particles) = 0;
      virtual Vec constraint_gradient(size_t der, InParticles particles) = 0;
};

class CollisionConstraint : public Constraint
{
public:
      CollisionConstraint(float distance, std::vector<EntityId> entities);

      float constraint(InParticles particles) override;
      Vec constraint_gradient(size_t der, InParticles particles) override;

      float m_distance;
};

class PBDSystem : System<PBDParticle, Transform>
{
public:
      void start() override;
      void awake(EntityId id) override;
      void update(float dt) override;

      template<typename C> C* add_constraint(std::vector<EntityId> particles)
      {
            m_constraints.emplace_back(new C(1.f, particles));
            return dynamic_cast<C*>(m_constraints.back());
      }
      template<typename C> C* add_constraint(std::vector<EntityId> particles, ConstraintType type)
      {
            auto constraint = dynamic_cast<Constraint*>(add_constraint<C>(particles));
            constraint->m_type = type;
            return dynamic_cast<C*>(constraint);
      }
private:
      PBDParticle& get_particle(EntityId id);
      Vec external_force(Vec pos);

      void damp_velocities();
      void velocity_update();
      void generate_collision_constraints();

      void solve_constraints();

      const int m_solverIterations = 2;
      void solve_seidel_gauss();

      std::vector<Constraint*> m_constraints;
};