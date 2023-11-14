#include "thread_pool.h"

#include <iostream>

void ThreadPool::initialize(int threads)
{
    if (m_initialized)
        return;
    m_initialized = true;
    shutdown_ = false;
    m_activeJobCount = 0;
    // Create the specified number of threads
    threads_.reserve(threads);
    for (int i = 0; i < threads; ++i)
        threads_.emplace_back(std::bind(&ThreadPool::threadEntry, this, i));
}
ThreadPool::ThreadPool() : m_initialized{ false }
{}
ThreadPool::~ThreadPool()
{
    {
        // Unblock any threads and tell them to stop
        std::unique_lock <std::mutex> l(lock_);

        shutdown_ = true;
        condVar_.notify_all();
    }

    // Wait for all threads to stop
    std::cerr << "Joining threads" << std::endl;
    for (auto& thread : threads_)
        thread.join();
}
void ThreadPool::doJob(std::function <void(void)> func)
{
    if (!m_initialized)
        return;
    // Place a job on the queue and unblock a thread
    std::unique_lock <std::mutex> l(lock_);

    jobs_.emplace(std::move(func));
    condVar_.notify_one();
}
void ThreadPool::threadEntry(int i)
{
    std::function <void(void)> job;

    while (1)
    {
        {
            std::unique_lock <std::mutex> l(lock_);

            while (!shutdown_ && jobs_.empty())
            {
                // std::cout << "thread " << i << " is waiting\n";
                condVar_.wait(l);
            }

            if (jobs_.empty())
            {
                // No jobs to do and we are shutting down
                // std::cout << "thread " << i << " terminates" << std::endl;
                return;
            }

            // std::cout << "thread " << i << " does a job" << std::endl;
            job = std::move(jobs_.front());
            jobs_.pop();
            m_activeJobCount++;
        }

        // Do the job without holding any locks
        job();
        m_activeJobCount--;

        if (!m_activeJobCount || shutdown_)
        {
            std::unique_lock<std::mutex> l(m_allFinishedLock);
            m_allFinished.notify_all();
        }
    }

}
void ThreadPool::wait_for_finish()
{
    std::unique_lock<std::mutex> l(m_allFinishedLock);
    while (!jobs_.empty())
    {
        m_allFinished.wait(l);
    }
}
