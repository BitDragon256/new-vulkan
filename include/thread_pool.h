#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>


class ThreadPool
{
public:

    ThreadPool();
    ~ThreadPool();
    void initialize(int threads);
    void doJob(std::function <void(void)> func);

protected:

    void threadEntry(int i);

    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    bool m_initialized;
    std::queue <std::function <void(void)>> jobs_;
    std::vector <std::thread> threads_;
};