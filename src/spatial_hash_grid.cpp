#include "spatial_hash_grid.h"

#include <math.h>

#include <algorithm>

#include "logger.h"

SpatialHashGrid::SpatialHashGrid(float gridSize) :
      m_gridSize{ gridSize }
{
      m_buckets.resize(100*100*10);
}

size_t SpatialHashGrid::hash_vec(Vector3 v)
{
      return ((
            (static_cast<int64_t>(std::floor(v.x / m_gridSize)) * 73856093)
            ^ (static_cast<int64_t>(std::floor(v.y / m_gridSize)) * 19349663)
            ^ (static_cast<int64_t>(std::floor(v.z / m_gridSize)) * 83492791)
            ) % m_buckets.size() + m_buckets.size()) % m_buckets.size();
}
size_t SpatialHashGrid::bucket_index(Vector3 v)
{
      return hash_vec(v);
}

std::vector<EntityId>& SpatialHashGrid::bucket_at(Vector3 pos)
{
      return m_buckets[bucket_index(pos)];
}
std::vector<EntityId>& SpatialHashGrid::bucket_at(Vector2 pos)
{
      return bucket_at(vec23(pos));
}

void SpatialHashGrid::surrounding_particles(Vector3 pos, std::vector<EntityId>& particles)
{
      for (int x = -1; x <= 1; x++)
      {
            for (int y = -1; y <= 1; y++)
            {
                  for (int z = -1; z <= 1; z++)
                  {
                        const auto& bucket = bucket_at(pos + Vector3 { x, y, z } * m_gridSize);
                        particles.insert(particles.end(), bucket.cbegin(), bucket.cend());
                  }
            }
      }
      //std::set<int> s;
      //size_t size = particles.size();
      //for (size_t i = 0; i < size; ++i) s.insert(particles[i]);
      //particles.assign(s.begin(), s.end());

      std::sort(particles.begin(), particles.end());
      particles.erase(std::unique(particles.begin(), particles.end()), particles.end());
}
void SpatialHashGrid::surrounding_particles(Vector2 pos, std::vector<EntityId>& particles)
{
      for (int dim = 0; dim < 2; dim++)
      {
            for (int sign = -1; sign <= 1; sign += 2)
            {
                  if (dim == 2 && sign == 1)
                        break;
                  const auto& bucket = bucket_at(pos + m_gridSize * Vector2 { (dim == 0) * sign, (dim == 1) * sign });
                  particles.insert(particles.end(), bucket.cbegin(), bucket.cend());
            }
      }
      const auto& bucket = bucket_at(pos);
      particles.insert(particles.end(), bucket.cbegin(), bucket.cend());
}
void SpatialHashGrid::insert_particle(Vector3 pos, EntityId id)
{
      bucket_at(pos).push_back(id);
}
void SpatialHashGrid::insert_particle(Vector2 pos, EntityId id)
{
      insert_particle(vec23(pos), id);
}

void SpatialHashGrid::change_particle(Vector3 oldPos, Vector3 newPos, EntityId id)
{
      if (bucket_index(oldPos) == bucket_index(newPos))
            return;
      remove_particle(oldPos, id);
      insert_particle(newPos, id);
}
void SpatialHashGrid::change_particle(Vector2 oldPos, Vector2 newPos, EntityId id)
{
      change_particle(vec23(oldPos), vec23(newPos), id);
}

void SpatialHashGrid::remove_particle(Vector3 pos, EntityId id)
{
      std::erase(bucket_at(pos), id);
}
void SpatialHashGrid::remove_particle(Vector2 pos, EntityId id)
{
      remove_particle(vec23(pos), id);
}

void SpatialHashGrid::print_buckets(Vector2 max)
{
      std::cout << "\n";
      for (float y = -max.y; y < max.y; y += m_gridSize)
      {
            std::cout << static_cast<size_t>(y / m_gridSize) << ": |";
            for (float x = -max.x; x < max.x; x += m_gridSize)
            {
                  size_t s = bucket_at({ x, y }).size();
                  if (s == 0)
                        std::cout << " ";
                  else
                        std::cout << s;
                  std::cout << "|";
            }
            std::cout << "\n";
      }
}
