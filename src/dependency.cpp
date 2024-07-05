#include "dependency.h"

#include <algorithm>

#include "logger.h"

Dependency::Dependency() :
      m_dependencies{ }, m_dependents{ }, m_resolved{ false }, m_isLoopDependency{ false }
{}

bool Dependency::try_update()
{
      for (auto [type, deps] : m_dependencies)
            for (auto dep : deps)
                  if (!dep.dependency->resolved())
                        return false;

      on_update();
      resolve();

      for (auto& [type, deps] : m_dependents)
      {
            logger::log(dependency_id() + " -> " + type);
            bool failed = true;

            for (auto dep : deps)
                  failed &= !dep.dependency->try_update();

            if (failed)
                  logger::log("\033[F\r" + dependency_id() + " -> " + type + " (failure)");
      }

      return true;
}
void Dependency::unresolve()
{
      for (auto [type, deps] : m_dependents)
            for (auto dep : deps)
                  dep.dependency->unresolve();

      on_unresolve();
      m_resolved = false;
}
bool Dependency::resolved() const
{
      return m_resolved;
}
void Dependency::resolve()
{
      m_resolved = true;
}
void Dependency::add_dependency_ref(DependencyRef dependency)
{
      push_to_map(m_dependencies, dependency);
      if (!m_isLoopDependency)
      {
            m_isLoopDependency = true;
            dependency.dependency->add_dependent(Reference(this));
            m_isLoopDependency = false;
      }
}
void Dependency::add_dependent_ref(DependencyRef dependent)
{
      push_to_map(m_dependents, dependent);
      if (!m_isLoopDependency)
      {
            m_isLoopDependency = true;
            dependent.dependency->add_dependency(Reference(this));
            m_isLoopDependency = false;
      }
}
void Dependency::push_to_map(std::unordered_map<DependencyId, std::vector<DependencyRef>>& map, DependencyRef dependency)
{
      if (
            map.contains(dependency.id)
      )
      {
            auto& dependencyList = map[dependency.id];
            if (std::find(
                  map[dependency.id].begin(),
                  map[dependency.id].end(),
                  dependency
            ) == map[dependency.id].end())
                  dependencyList.push_back(dependency);
      }
      else
      {
            auto& dependencyList = map[dependency.id];
            dependencyList.clear();
            dependencyList.push_back(dependency);
      }
}

// -----------------------
// DEPENDENCY REF
// -----------------------

bool operator== (const DependencyRef& a, const DependencyRef& b)
{
      return a.id == b.id && a.dependency == b.dependency;
}
