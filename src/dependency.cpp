#include "dependency.h"

Dependency::Dependency() :
      m_dependencies{ }, m_dependents{ }, m_resolved{ false }
{}

bool Dependency::try_update()
{
      for (auto [type, deps] : m_dependencies)
            for (auto dep : deps)
                  if (!dep->resolved())
                        return false;

      on_update();
      resolve();

      for (auto& [type, deps] : m_dependents)
            for (auto dep : deps)
                  dep->try_update();

      return true;
}
bool Dependency::resolved() const
{
      return m_resolved;
}
void Dependency::resolve()
{
      m_resolved = true;
}
void Dependency::add_dependency(DependencyRef dependency, typeName type)
{
      if (!m_dependencies.contains(type))
      {
            push_to_map(m_dependencies, dependency, type);
            dependency->add_dependent(DependencyRef(this), type);
      }
}
void Dependency::add_dependent(DependencyRef dependent, typeName type)
{
      if (!m_dependents.contains(type))
      {
            push_to_map(m_dependents, dependent, type);
            dependent->add_dependency(DependencyRef(this), type);
      }
}
void Dependency::add_dependencies(std::initializer_list<std::pair<std::initializer_list<DependencyRef>, typeName>> dependencies)
{
      for (auto deps : dependencies)
            for (auto dep : deps.first)
                  add_dependency(dep, deps.second);
}
void Dependency::push_to_map(std::unordered_map<typeName, std::vector<DependencyRef>>& map, DependencyRef dependency, typeName type)
{
      if (map.contains(type))
            map[type].push_back(dependency);
      else
      {
            auto& dependencyList = map[type];
            dependencyList = std::vector<DependencyRef>();
            dependencyList.push_back(dependency);
      }
}
