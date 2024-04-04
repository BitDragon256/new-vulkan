#include "dependency.h"

Dependency::Dependency() :
      m_dependencies{ }, m_dependents{ }, m_resolved{ false }
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
            for (auto dep : deps)
                  dep.dependency->try_update();

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
void Dependency::add_dependency(DependencyRef dependency)
{
      if (!m_dependencies.contains(dependency.type))
      {
            push_to_map(m_dependencies, dependency);
            dependency.dependency->add_dependent(make_dependency_ref(this));
      }
}
void Dependency::add_dependent(DependencyRef dependent)
{
      if (!m_dependents.contains(dependent.type))
      {
            push_to_map(m_dependents, dependent);
            dependent.dependency->add_dependency(make_dependency_ref(this));
      }
}
void Dependency::add_dependencies(std::initializer_list<DependencyRef> dependencies)
{
      for (auto dep : dependencies)
            add_dependency(dep);
}
void Dependency::push_to_map(std::unordered_map<typeName, std::vector<DependencyRef>>& map, DependencyRef dependency)
{
      if (map.contains(dependency.type))
            map[dependency.type].push_back(dependency);
      else
      {
            auto& dependencyList = map[dependency.type];
            dependencyList.clear();
            dependencyList.push_back(dependency);
      }
}
