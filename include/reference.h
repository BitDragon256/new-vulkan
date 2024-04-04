#pragma once

#include <memory>

#define REF(T) Reference<T>

// basically just a pointer wrapper
template<typename T>
class Reference
{
public:
      Reference() :
            m_empty{ true }, m_pointer{ nullptr }
      {}
      Reference(T* t) :
            m_empty{ false }, m_pointer{ t }
      {}

      T operator* ()
      {
            return *m_pointer;
      }
      T* operator-> ()
      {
            return m_pointer;
      }
      
      T* get() const
      {
            return m_pointer;
      }
      bool empty() const
      {
            return m_empty;
      }

private:
      bool m_empty;
      T* m_pointer;
};

template<typename T>
struct std::hash<Reference<T>>
{
      std::size_t operator()(const Reference<T>& s) const noexcept
      {
            return std::hash<T*>{}(s.get()) ^ (std::hash<bool>{}(s.empty()));
      }
};

template<typename T>
bool operator==(const Reference<T>& a, const Reference<T>& b)
{
      return a.empty() == b.empty() && a.get() == b.get();
}