#pragma once

#include "../include/rasteriser.h"
#include "../maths/vec.hpp"
#include <array>
#include <barrier>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <latch>
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

template <typename T> class LockedQueue
{

    class QueueNode
    {
      public:
        T                          data;
        std::unique_ptr<QueueNode> next;
        QueueNode() : data{}, next{nullptr}
        {
        }
        QueueNode(T data) : data{data}, next{nullptr}
        {
        }
        ~QueueNode()
        {
        }
    };

    std::unique_ptr<QueueNode> head;
    QueueNode                 *tail;
    std::mutex                 head_mutex{};
    std::mutex                 tail_mutex{};

  public:
    LockedQueue()
        // dummy node
        : head{new QueueNode}, tail{head.get()}
    {
    }

    void push(T data)
    {
        auto ptr = std::unique_ptr<QueueNode>(new QueueNode{}); // Create a dummy node
        // Use previously created dummy node to push new data on
        std::scoped_lock tm(tail_mutex);
        tail->data = data;
        tail->next = std::move(ptr);
        tail       = (tail->next).get();
    }

    QueueNode *get_tail_ts()
    {
        std::scoped_lock tl(tail_mutex);
        return tail;
    }
    bool pop(T &item)
    {
        std::scoped_lock hm(head_mutex);
        if (head.get() == get_tail_ts())
            return false;
        // While popping retrieve data form the tail node
        auto data = head->data;
        head      = std::move(head->next);
        item      = data;
        return true;
    }

    bool empty() const
    {
        std::scoped_lock hm(head_mutex);
        return head.get() == get_tail_ts();
    }

    ~LockedQueue()
    {
    }
};

// A drop in replacement for std::function object
// A super fast static queue
template <typename T, size_t N> class StaticQueue
{
    T          data[N];
    std::mutex mut{};

  public:
    size_t head   = 0;
    size_t tail   = 0;
    StaticQueue() = default;

    void push(T item)
    {
        std::scoped_lock l(mut);
        data[tail++] = item;
    }

    bool pop(T &item)
    {
        // lock free programming
        // TODO::make this lock free
        std::scoped_lock s(mut);
        if (head == tail)
            return false;
        if (head > 2)
            __debugbreak();
        item = data[head++];
        return true;
    }

    size_t size() const
    {
        return head - tail;
    }

    bool empty() const
    {
        return size() == 0;
    }
    void clear()
    {
        std::scoped_lock l(mut);
        head = 0;
        tail = 0;
    }
};

// But first thread safe queue data structure
// Naive mutex based thread safe queue
// Joke queue
template <typename T, size_t N> class ThreadSafeQueue
{
    std::mutex mut{};
    // std::queue<T> queue{};
    LockedQueue<T> queue{};

  public:
    // StaticQueue<T, N> queue{};
    ThreadSafeQueue() = default;

    void push(T const &item)
    {
        queue.push(item);
    }

    bool pop(T &item)
    {
        return queue.pop(item);
        /*if (queue.empty())
            return false;
        item = queue.front();
        queue.pop();
        return true;*/
    }

    size_t getsize()
    {
        // std::scoped_lock(mut);
        // return queue.size();
        return 0;
    }
    void clear()
    {
        // queue.clear();
    }
};

// so going with standard threads

class ThreadPool
{
    std::atomic<bool> done          = true;
    size_t            no_of_threads = 0;
    // Since we don't have thread safe queue, we going to use std::queue along with a mutex
    std::vector<std::thread> worker_threads;
    constexpr static size_t  THREAD_POOL_QUEUE_SIZE = 25;

  public:
    // std::atomic<uint8_t> counter = 0;
    // pointers to the rescue .. wonder what other languages use instead of pointer
    std::latch *latch       = nullptr;
    using ThreadPoolFuncPtr = void (*)(void *); // Function returning void and taking void ptr
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
    ThreadSafeQueue<ThreadPool::ThreadPoolFunc, THREAD_POOL_QUEUE_SIZE> task_queue{};

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
                // Signals that this thread's work has finished
                latch->count_down();
            }
            /*else
                std::this_thread::yield();*/
        }
    }

    void add_task(ThreadPoolFunc const &task)
    {
        task_queue.push(task);
    }
    // Poor man's signal handling
    uint8_t finished() const
    {
        // return counter.load();
        return 0;
    }
    void wait_till_finished()
    {
        latch->wait();
    }
    void started(std::latch *wait_till)
    {
        // counter.store(0);
        latch = wait_till;
        task_queue.clear();
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

template <typename T, size_t N> class StaticLockFreeQueue
{
    T                   data[N];
    std::atomic<size_t> head = 0;
    std::atomic<size_t> tail = 0;

  public:
    StaticLockFreeQueue() = default;

    void push(T item)
    {
        auto t = tail.load();
        do
        {
            data[t + 1] = std::move(item);
        } while (!tail.compare_exchange_weak(t, t + 1));
    }

    bool pop(T const &ref)
    {
        // Lock free implementation
        auto   h   = head.load();
        size_t len = 0;
        do
        {
            len = h - tail.load();
            if (!len)
                return false;
            ref = data[h + 1];
        } while (!head.compare_exchange_weak(h, h + 1));
        return true;
    }

    size_t size() const
    {
        return (tail - head);
    }

    bool empty() const
    {
        return (tail - head) == 0;
    }
    void clear()
    {
        head = tail = 0;
    }
};

// Another thread pool that doesn't need mutex based protection or lock free mechanism

namespace Alternative
{
class ThreadPool
{
    std::vector<std::thread> worker_threads;
    std::atomic<bool>        done = true;

    constexpr static size_t  N = 4;

  public:
    std::latch *latch       = nullptr;
    using ThreadPoolFuncPtr = void (*)(void *); // Function returning void and taking void ptr
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

    struct AlternativeTaskDesc
    {
        std::atomic<bool> completed = true;
        ThreadPoolFunc    task;
        AlternativeTaskDesc() = default;
        AlternativeTaskDesc(bool b, const ThreadPoolFunc& fn) : completed{b}, task{fn}
        {
        }
        AlternativeTaskDesc(const AlternativeTaskDesc &lhs)
        {
            completed.store(lhs.completed.load());
            task = lhs.task;
        }
        AlternativeTaskDesc &operator=(const AlternativeTaskDesc &lhs)
        {
            completed.store(lhs.completed.load());
            task = lhs.task;
            return *this;
        }
    };

  private:
    std::vector<AlternativeTaskDesc> Task1;
    std::vector<AlternativeTaskDesc> Task2;

  public:
    // Since above vectors are guarranted to never resize, task_ptr is always valid
    std::atomic<std::vector<AlternativeTaskDesc> *> task_ptrActive;
    std::atomic<std::vector<AlternativeTaskDesc> *> task_ptrPassive;

    ThreadPool() noexcept
    {
        Task1 = std::vector<AlternativeTaskDesc>(N, AlternativeTaskDesc{});
        Task2 = std::vector<AlternativeTaskDesc>(N, AlternativeTaskDesc{});
        task_ptrActive.store(&Task1);
        task_ptrPassive.store(&Task2);
        for (unsigned int i = 0; i < N; ++i)
            worker_threads.push_back(std::thread{&std::remove_reference_t<decltype(*this)>::worker_threads_func, this});
    }
    ThreadPool(unsigned int no_of_threads) noexcept
    {
    }

    void worker_threads_func()
    {
        while (done.load())
        {
            auto &current_pool = *(task_ptrActive.load());
            for (auto &work : current_pool)
            {
                if (!work.completed.exchange(true))
                {
                    work.task();
                    latch->count_down();
                }
            }
            std::this_thread::yield();
        }
    }

    // void add_tasks(Parallel::ParallelRenderer::ParallelThreadArgStruct* argPtr, size_t length, ThreadPoolFuncPtr fn)
    //{
    //     //// Switch the pointer after here
    //     //auto i = 0;
    //     //for (auto& task : *task_ptrPassive)
    //     //{
    //     //    assert(i < length);
    //     //    AlternativeTaskDesc desc;
    //     //    desc.completed = false;
    //     //    desc.task      = ThreadPoolFunc(fn, argPtr + i);
    //     //    task           = desc;
    //     //}
    // }
    //
    std::vector<AlternativeTaskDesc> *get_passive_ptr()
    {
        return task_ptrPassive;
    }
    uint8_t finished() const
    {
        return 0;
    }
    void wait_till_finished()
    {
        latch->wait();
    }
    void started(std::latch *wait_till)
    {
        // The swap here needn't be atomic since only active will be used for the time being by running threads 
        
        auto temp = task_ptrActive.exchange(task_ptrPassive);
        task_ptrPassive.exchange(temp);
        latch = wait_till;
    }

    ~ThreadPool()
    {
        done.store(false);
        for (auto &thread : worker_threads)
        {
            if (thread.joinable())
                thread.join();
        }
    }
};
} // namespace Alternative