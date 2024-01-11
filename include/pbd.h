#pragma once

#include <array>
#include <vector>
#include <functional>

#include "ecs.h"
#include "model-handler.h"
#include "spatial_hash_grid.h"
#include "gui.h"

#include "nve_types.h"
#include "profiler.h"

typedef float Real;
typedef size_t Cardinality;

#define PBD_3D

#ifdef PBD_3D

typedef Vector3 Vec;

#else

typedef Vector2 Vec;

#endif

struct PBDParticle
{
      PBDParticle();
      Vec oldPosition;
      Vec tempPosition;
      Vec position;
      Vec velocity;
      float mass;
      float invmass;
      float radius;

      Vec scale;

      float density;
      float fluidMass;
      float scalingFactor;
};

GUI_PRINT_COMPONENT_START(PBDParticle)

ImGui_DragVector("position", component.position);
ImGui_DragVector("velocity", component.velocity);
ImGui::DragFloat("mass", &component.mass);
ImGui::DragFloat("invmass", &component.invmass);
ImGui::DragFloat("density", &component.density);

GUI_PRINT_COMPONENT_END

enum ConstraintType { Equality, Inequality, InverseInequality };

typedef std::vector<PBDParticle*> InParticles;
class Constraint
{
public:
      Constraint(Cardinality cardinality, std::vector<EntityId> entities, ECSManager* ecs);

      const Cardinality m_cardinality;
      float m_compliance;
      std::vector<EntityId> m_entities;
      std::vector<PBDParticle*> m_particles;
      std::vector<Vec> m_gradients;

      ConstraintType m_type;
      virtual float constraint(InParticles particles) = 0;
      virtual Vec constraint_gradient(size_t der, InParticles particles) = 0;
};

class PBDSystem;
class ConstraintGenerator
{
public:
      virtual std::vector<Constraint*> create(
            EntityId particle, std::vector<EntityId> surrounding,
            ECSManager* ecs
      ) = 0;
};

class CollisionConstraint : public Constraint
{
public:
      CollisionConstraint(float distance, std::vector<EntityId> entities, ECSManager* ecs);

      float constraint(InParticles particles) override;
      Vec constraint_gradient(size_t der, InParticles particles) override;

      float m_distance;
};
class CollisionConstraintGenerator : public ConstraintGenerator
{
public:
      std::vector<Constraint*> create(
            EntityId particle, std::vector<EntityId> surrounding,
            ECSManager* ecs
      ) override;

      float m_radius;
};

#define PBD_GRID_SIZE 1.1f

class PBDSystem : System<PBDParticle, Transform>
{
public:
      PBDSystem();
      void start() override;
      void awake(EntityId id) override;
      void update(float dt) override;

      template<typename C> C* add_constraint(std::vector<EntityId> particles)
      {
            m_constraints.emplace_back(new C(1.f, particles, m_ecs));
            return dynamic_cast<C*>(m_constraints.back());
      }
      template<typename C> C* add_constraint(std::vector<EntityId> particles, ConstraintType type)
      {
            auto constraint = dynamic_cast<Constraint*>(add_constraint<C>(particles));
            constraint->m_type = type;
            return dynamic_cast<C*>(constraint);
      }
      void register_self_generating_constraint(ConstraintGenerator* generator);

      float m_dampingConstant = 0.995f;
      int m_solverIterations = 5;
      int m_substeps = 1;

private:
      PBDParticle& get_particle(EntityId id);
      Vec external_force(Vec pos);

      void pbd_update(float dt);
      void xpbd_update(float dt);
      void xpbd_substep(float dt);

      void draw_debug_lines();
      void sync_transform();

      void damp_velocities();
      void velocity_update();

      void generate_constraints();
      std::vector<ConstraintGenerator*> m_constraintGenerators;

      void solve_constraints();

      void solve_seidel_gauss();
      void xpbd_solve(float dt);
      void solve_sys();

      std::vector<Constraint*> m_constraints;
      size_t m_constraintStart;

      SpatialHashGrid m_grid;
      void sync_grid(PBDParticle& particle, EntityId entity);

      Profiler m_profiler;
};