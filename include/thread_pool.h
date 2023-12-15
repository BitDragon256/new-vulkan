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
    void wait_for_finish();

protected:

    void threadEntry(int i);

    std::mutex lock_;
    std::mutex m_allFinishedLock;
    std::condition_variable condVar_;
    std::condition_variable m_allFinished;
    bool shutdown_;
    bool m_initialized;
    std::queue <std::function <void(void)>> jobs_;
    std::mutex m_activeJobCountMutex;
    size_t m_activeJobCount;
    void change_job_count(int delta);
    std::vector <std::thread> threads_;
};