#pragma once

#include "core/containers/vector.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace kw {

class Task;
class TaskNode;

class TaskScheduler {
public:
    TaskScheduler(MemoryResource& persistent_memory_resource, size_t thread_count);
    ~TaskScheduler();

    // Start running the given task when all its dependencies have completed.
    void enqueue_task(MemoryResource& transient_memory_resource, Task* task);

    // Help worker threads scheduling tasks. Return when there's no tasks left and all worker threads have completed.
    void join();

    // Only the number of worker threads is returned. Main thread may join task scheduling too.
    size_t get_thread_count() const;

private:
    void worker_thread(size_t thread_index);
    void run_task(Task* task);

    std::mutex m_mutex;
    TaskNode* m_ready_tasks;
    size_t m_busy_threads;
    std::condition_variable m_ready_task_condition_variable;
    std::condition_variable m_busy_thread_condition_variable;

    std::atomic<bool> m_is_running;

    Vector<std::thread> m_threads;
};

} // namespace kw
