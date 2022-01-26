#pragma once

#include "../include/rasteriser.h"
#include "../maths/vec.hpp"
#include <array>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <ranges>
#include <semaphore>
#include <thread>
#include <vector>

// Joinable threads
// No interrupt capability needed for thread pool
// std::jthread

// Now let's move the parallelism to screen level instead of just rasterisation level
// May need to update allocator to be threadsafe .. later
// Better to use thread local allocator for that purpose :D
//
// Helper templates

// Storing std::function is doing a lot of heap allocation .. avoid that using type erasure

template <typename T, typename... Args> constexpr T vMin(T arg1, T arg2, Args... args)
{
    if constexpr (sizeof...(args) == 0)
        return std::min(arg1, arg2);
    else
        return std::min(arg1, vMin(arg2, args...));
}

template <typename T, typename... Args> constexpr T vMax(T arg1, T arg2, Args... args)
{
    if constexpr (sizeof...(args) == 0)
        return std::max(arg1, arg2);
    else
        return std::max(arg1, vMax(arg2, args...));
}

// A drop in replacement for std::function object

// But first thread safe queue data structure
// Naive mutex based thread safe queue
// Joke queue
template <typename T> class ThreadSafeQueue
{
    std::mutex    mut{};
    std::queue<T> queue{};

  public:
    ThreadSafeQueue() = default;

    void push(T const &item)
    {
        std::scoped_lock s(mut);
        queue.push(item);
    }

    bool pop(T &item)
    {
        std::scoped_lock s(mut);
        if (queue.empty())
            return false;
        item = queue.front();
        queue.pop();
        return true;
    }

    size_t getsize()
    {
        std::scoped_lock(mut);
        return queue.size();
    }
};

// so going with standard threads

class ThreadPool
{
    std::atomic<bool> done          = true;
    unsigned int      no_of_threads = 0;
    // Since we don't have thread safe queue, we going to use std::queue along with a mutex
    std::vector<std::thread> worker_threads;

  public:
    std::atomic<uint8_t> counter = 0;

    using ThreadPoolFuncPtr      = void (*)(void *); // Function returning void and taking void ptr
    struct ThreadPoolFunc
    {
        // Function to pointer
        // void* arguments which will be back typecasted during each run
        ThreadPoolFuncPtr fn;
        void             *args; // Type erased argument to above function
        void              operator()()
        {
            fn(args);
        }
    };
    ThreadSafeQueue<ThreadPool::ThreadPoolFunc> task_queue;

    ThreadPool() noexcept : no_of_threads{std::thread::hardware_concurrency() - 2}
    {
        for (unsigned int i = 0; i < no_of_threads; ++i)
            worker_threads.push_back(std::thread{&std::remove_reference_t<decltype(*this)>::worker_threads_func, this});
    }
    ThreadPool(unsigned int no_of_threads) noexcept : no_of_threads{no_of_threads}
    {
    }

    void worker_threads_func()
    {
        while (done.load())
        {
            ThreadPoolFunc task;
            if (task_queue.pop(task))
            {
                task();
                counter++;
            }
        }
    }

    void add_task(ThreadPoolFunc const &task)
    {
        task_queue.push(task);
    }
    // Poor man's signal handling
    uint8_t finished() const
    {
        return counter.load();
    }
    void started()
    {
        counter.store(0);
    }

    ~ThreadPool()
    {
        // wait until all task are zero
        using namespace std::chrono;
        while (task_queue.getsize() != 0)
        {
            std::this_thread::sleep_for(0.2s);
        };
        done.store(false);
        for (auto &thread : worker_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }

  public:
};

// Implement parallel rendering using all the hardware concurrent threads for maximum performance along with SIMD
// to boost up the fps really high

// Each thread will process every triangles simultaneously but will render to only their available region
// No segmented rendering

// For example, for 2 thread working on framebuffer
// The buffer will be divided into 2 parts and each thread will handle each part individually
// A thread can operate on each triangle parallely, but that creates invalid color buffer synchronization
// Each triangle can be operated on parallely, but SIMD is really fine for that stuffs

// } // namespace Parallel