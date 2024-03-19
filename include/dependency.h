#pragma once

#include <vector>

#include "reference.h"

class Dependency;

typedef Reference<Dependency> DependencyRef;

class Dependency
{
public:

      Dependency();
      bool try_update();
      virtual void on_update() = 0;
      bool resolved() const;
      void add_dependency(DependencyRef dependency);
      void add_dependent(DependencyRef dependent);
      void add_dependencies(std::initializer_list<DependencyRef> dependencies);

protected:

      std::vector<DependencyRef> m_dependencies;
      std::vector<DependencyRef> m_dependents;

private:

      void resolve();
      bool m_resolved;

};
