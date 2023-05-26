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

void log_cond_err(bool cond, std::string err)
{
    if (!cond)
    {
        throw std::runtime_error(err);
    }
}