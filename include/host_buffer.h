#pragma once
#ifndef HOST_BUFFER_H
#define HOST_BUFFER_H

#include <vector>

template<typename T>
class HostBuffer
{
public:
    [[nodiscard]] size_t size() const;
    void unload();
    void add_data(const std::vector<T>& data);

private:
    std::vector<T> m_data;
};

#endif //HOST_BUFFER_H
