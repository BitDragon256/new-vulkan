#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "ecs.h"
#include "nve_types.h"

class SpatialHashGrid
{
public:
      SpatialHashGrid(float gridSize);
      std::vector<EntityId>& bucket_at(Vector3 pos);
      std::vector<EntityId>& bucket_at(Vector2 pos);

      void surrounding_particles(Vector3 pos, std::vector<EntityId>& particles);
      void surrounding_particles(Vector2 pos, std::vector<EntityId>& particles);

      void insert_particle(Vector3 pos, EntityId id);
      void insert_particle(Vector2 pos, EntityId id);

      void change_particle(Vector3 oldPos, Vector3 newPos, EntityId id);
      void change_particle(Vector2 oldPos, Vector2 newPos, EntityId id);

      void remove_particle(Vector3 pos, EntityId id);
      void remove_particle(Vector2 pos, EntityId id);

      void print_buckets(Vector2 max);

private:
      const float m_gridSize;
      std::vector<std::vector<EntityId>> m_buckets;
      size_t hash_vec(Vector3 v);
      size_t bucket_index(Vector3 v);

      Vector3 vec23(Vector2 vec);
};