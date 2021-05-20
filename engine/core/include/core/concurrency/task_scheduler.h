#pragma once

#include <core/containers/vector.h>

#include <atomic>
#include <thread>

namespace kw {

class Task;
class TaskNode;

class TaskScheduler {
public:
    TaskScheduler(MemoryResource& persistent_memory_resource, size_t thread_count);
    ~TaskScheduler();

    void enqueue_task(MemoryResource& transient_memory_resource, Task* task);

    // Help worker threads scheduling tasks. Return when there's no tasks left and all worker threads have completed.
    void join();

    // Only the number of worker threads is returned. Main thread may join task scheduling too.
    size_t get_thread_count() const {
        return m_threads.size();
    }

private:
    void worker_thread();
    void run_task(Task* task);

    std::atomic<TaskNode*> m_ready_tasks;
    std::atomic<uint32_t> m_busy_threads;
    std::atomic<bool> m_is_running;

    Vector<std::thread> m_threads;
};

} // namespace kw
