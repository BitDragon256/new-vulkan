#include "dependency.h"

Dependency::Dependency() :
      m_dependencies{ }, m_dependents{ }, m_resolved{ false }
{}

bool Dependency::try_update()
{
      for (auto dep : m_dependencies)
            if (!dep->resolved())
                  return false;

      on_update();
      resolve();

      for (auto dep : m_dependents)
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
void Dependency::add_dependency(DependencyRef dependency)
{
      if (std::find(m_dependencies.begin(), m_dependencies.end(), dependency) == m_dependencies.end())
      {
            m_dependencies.push_back(dependency);
            dependency->add_dependent(DependencyRef(this));
      }
}
void Dependency::add_dependent(DependencyRef dependent)
{
      if (std::find(m_dependents.begin(), m_dependents.end(), dependent) == m_dependents.end())
      {
            m_dependents.push_back(dependent);
            dependent->add_dependency(DependencyRef(this));
      }
}
void Dependency::add_dependencies(std::initializer_list<DependencyRef> dependencies)
{
      for (auto dep : dependencies)
            add_dependency(dep);
}