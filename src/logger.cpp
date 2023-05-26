#include "logger.h"

#include <iostream>

void log(std::string str) {
    std::cout << str << '\n';
}
void log(int i) {
    std::cout << i << '\n';
}
void log(uint32_t i) {
    std::cout << i << '\n';
}
void log_now(std::string str) {
    std::cout << str << std::endl;
}