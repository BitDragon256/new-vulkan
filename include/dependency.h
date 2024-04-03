#pragma once

#include <string>
#include <unordered_map>

#include "reference.h"

class Dependency;

typedef Reference<Dependency> DependencyRef;
typedef std::string typeName;

template<typename T>
inline typeName type_to_name()
{
      return typeid(T).name();
}

class Dependency
{
public:

      Dependency();
      bool try_update();
      virtual void on_update() = 0;
      bool resolved() const;
      void add_dependency(DependencyRef dependency, typeName type);
      void add_dependent(DependencyRef dependent, typeName type);
      void add_dependencies(std::initializer_list<std::pair<std::initializer_list<DependencyRef>, typeName>> dependencies);

protected:

      std::unordered_map<typeName, std::vector<DependencyRef>> m_dependencies;
      std::unordered_map<typeName, std::vector<DependencyRef>> m_dependents;

      template<typename T>
      inline Reference<T> get_dependency()
      {
            return dynamic_cast<T*>(get_dependencies().first().get());
      }
      template<typename T>
      inline std::vector<Reference<T>> get_dependencies()
      {
            return m_dependencies[type_to_name<T>()];
      }

private:

      void resolve();
      bool m_resolved;

      void push_to_map(std::unordered_map<typeName, std::vector<DependencyRef>>& map, DependencyRef dependency, typeName type);

};
