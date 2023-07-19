#include "logger.h"

#include <iostream>
#include <stdexcept>

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
void log_now(std::string str)
{
    std::cout << str << std::endl;
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