#include "logger.h"

#include <iostream>
#include <stdexcept>

namespace logger
{

      void log_nnl(std::string str)
      {
            std::cout << str;
      }
      void log(std::string str)
      {
            std::cout << str << '\n';
      }
      void log(int i)
      {
            std::cout << i << '\n';
      }
      void log(uint32_t i)
      {
            std::cout << i << '\n';
      }
      void log(size_t i)
      {
            std::cout << i << '\n';
      }
      void log(float f)
      {
            std::cout << f << '\n';
      }
      void log_now(std::string str)
      {
            std::cout << str << std::endl;
      }
      void log(Vector2 vec)
      {
            std::cout << "x: " << vec.x << " y: " << vec.y << '\n';
      }
      void log(Vector3 vec)
      {
            std::cout << "x: " << vec.x << " y: " << vec.y << " z: " << vec.z << '\n';
      }
      void log_cond(bool cond, std::string str)
      {
            if (cond)
            {
                  log(str);
            }
      }

      void log_err(std::string err)
      {
            log(err);
            throw std::runtime_error(err);
      }
      void log_cond_err(bool cond, std::string err)
      {
            if (!cond)
            {
                  log_err(err);
            }
      }

} // namespace logger