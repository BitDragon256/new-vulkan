#pragma once

#include <string>
#include <unordered_map>

#include "reference.h"

class Dependency;

typedef std::string DependencyId;

template<typename T>
inline DependencyId type_to_id()
{
      T t;
      return t.dependency_id();
}

// typedef Reference<Dependency> DependencyRef;
typedef struct DependencyRef_T
{
      Reference<Dependency> dependency;
      DependencyId id;
} DependencyRef;

bool operator== (const DependencyRef& a, const DependencyRef& b);

template<typename T>
inline DependencyRef make_dependency_ref(Reference<T> object)
{
      DependencyRef ref = {};
      ref.id = object->dependency_id();
      ref.dependency = Reference<Dependency>(object.get());
      return ref;
}

class Dependency
{
public:

      Dependency();
      bool try_update();
      void unresolve();
      bool resolved() const;

      template<typename T>
      void add_dependency(REF(T) dependency)
      {
            add_dependency_ref(make_dependency_ref<T>(dependency));
      }
      template<typename T>
      void add_dependent(REF(T) dependent)
      {
            add_dependent_ref(make_dependency_ref<T>(dependent));
      }
      template<typename T>
      void add_dependencies(std::initializer_list<REF(T)> dependencies)
      {
            for (auto dep : dependencies)
                  add_dependency_ref(make_dependency_ref(dep));
      }
      template<typename T>
      void add_dependencies(std::vector<REF(T)> dependencies)
      {
            for (auto dep : dependencies)
                  add_dependency_ref(make_dependency_ref(dep));
      }

      virtual DependencyId dependency_id() = 0;

protected:

      virtual void on_update() = 0;
      virtual void on_unresolve() = 0;

      bool dependency_try_update();

      std::unordered_map<DependencyId, std::vector<DependencyRef>> m_dependencies;
      std::unordered_map<DependencyId, std::vector<DependencyRef>> m_dependents;

      template<typename T>
      inline std::vector<Reference<T>> get_dependencies()
      {
            std::vector<Reference<T>> deps;
            for (DependencyRef& dep : m_dependencies[type_to_id<T>()])
                  deps.push_back(Reference(dynamic_cast<T*>(dep.dependency.get())));
            return deps;
      }
      template<typename T>
      inline Reference<T> get_dependency()
      {
            return get_dependencies<T>().front();
      }

      bool m_blockUpdate;

private:

      void resolve();
      bool m_resolved;

      bool m_isLoopDependency;

      void push_to_map(std::unordered_map<DependencyId, std::vector<DependencyRef>>& map, DependencyRef dependency);

      void add_dependency_ref(DependencyRef dependency);
      void add_dependent_ref(DependencyRef dependent);

};
