#pragma once
#ifndef HOST_BUFFER_H
#define HOST_BUFFER_H

#include <vector>

#include "reference.h"

template<typename T>
class HostBuffer
{
public:
    [[nodiscard]] size_t size() const;
    void unload();
    void add_data(const std::vector<T>& data);

    T& operator[](size_t index) {
        return m_data[index];
    }
    [[nodiscard]] const T& operator[](size_t index) const {
        return m_data[index];
    }

private:
    std::vector<T> m_data;
};

template<typename T>
class HostBufferView
{
public:
    REF(HostBuffer<T>) m_hostBuffer;
};

template<typename T>
class HostBufferSingleView
{
public:
    REF(HostBuffer<T>) m_hostBuffer;

    HostBufferSingleView(const Reference<HostBuffer<T>> &hostBuffer, size_t index)
        : m_hostBuffer(hostBuffer),
          index(index) {}

    explicit operator T()
    {
        return m_hostBuffer[index];
    }
    HostBufferSingleView& operator=(const T& t)
    {
        m_hostBuffer[index] = t;
        return *this;
    }
private:
    size_t index;
};

#endif //HOST_BUFFER_H
