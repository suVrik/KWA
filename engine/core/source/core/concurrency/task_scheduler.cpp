#include "core/concurrency/task_scheduler.h"
#include "core/concurrency/concurrency_utils.h"
#include "core/concurrency/task.h"
#include "core/concurrency/task_node.h"
#include "core/debug/assert.h"

namespace kw {

TaskScheduler::TaskScheduler(MemoryResource& persistent_memory_resource, size_t thread_count)
    : m_ready_tasks(nullptr)
    , m_busy_threads(0)
    , m_is_running(true)
    , m_threads(persistent_memory_resource)
{
    m_threads.reserve(thread_count);
    for (size_t i = 0; i < thread_count; i++) {
        m_threads.push_back(std::thread(&TaskScheduler::worker_thread, this));

        char name_buffer[24];
        sprintf_s(name_buffer, sizeof(name_buffer), "Worker Thread %zu", i);
        ConcurrencyUtils::set_thread_name(m_threads.back(), name_buffer);
    }
}

TaskScheduler::~TaskScheduler() {
    m_is_running = false;

    for (std::thread& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void TaskScheduler::enqueue_task(MemoryResource& transient_memory_resource, Task* task) {
    // Each task has one extra dependency by default so it doesn't run before it's scheduled here.
    // If task has more dependencies, completion of those dependencies will trigger this task's run.
    size_t input_dependency_count = --task->m_input_dependency_count;
    if (input_dependency_count == 0) {
        // Task doesn't have any dependencies. Add it to ready stack.

        TaskNode* new_head = transient_memory_resource.allocate<TaskNode>();
        new_head->task = task;
        new_head->next = m_ready_tasks.load(std::memory_order_relaxed);

        while (!m_ready_tasks.compare_exchange_weak(new_head->next, new_head, std::memory_order_release, std::memory_order_relaxed));
    }
}

void TaskScheduler::join() {
    while (true) {
        TaskNode* head = m_ready_tasks.load(std::memory_order_relaxed);
        if (head != nullptr) {
            if (m_ready_tasks.compare_exchange_weak(head, head->next, std::memory_order_release, std::memory_order_relaxed)) {
                run_task(head->task);
            }
        } else {
            if (m_busy_threads == 0) {
                // There's no more tasks and all worker threads are idle (hence no more tasks from running worker threads too).
                return;
            }
        }

        std::this_thread::yield();
    }
}

void TaskScheduler::worker_thread() {
    while (m_is_running) {
        TaskNode* head = m_ready_tasks.load(std::memory_order_relaxed);
        if (head != nullptr) {
            // If this was the last task on ready stack, don't let the main thread leave from `join` method,
            // because this last task may spawn many other tasks.
            m_busy_threads++;

            if (m_ready_tasks.compare_exchange_weak(head, head->next, std::memory_order_release, std::memory_order_relaxed)) {
                run_task(head->task);
            }

            // If there's no more tasks on ready stack and all worker threads are idle,
            // main thread may leave the `join` method and start the next frame.
            m_busy_threads--;
        }

        std::this_thread::yield();
    }
}

void TaskScheduler::run_task(Task* task) {
    // Run task.

    task->run();

    // Notify future dependencies that this task has completed.

    TaskNode* task_dependency = task->m_output_dependencies.load(std::memory_order_relaxed);
    while (!task->m_output_dependencies.compare_exchange_strong(task_dependency, &INVALID_TASK_NODE, std::memory_order_release, std::memory_order_relaxed));

    // Notify past dependencies that this task has completed.

    while (task_dependency != nullptr) {
        // We may reuse `task_dependency` for ready stack and overwrite `next`.
        TaskNode* next_task_dependency = task_dependency->next;

        size_t input_dependency_count = --task_dependency->task->m_input_dependency_count;
        if (input_dependency_count == 0) {
            // Task doesn't have any dependencies left. Move it from dependency stack to ready stack.
            task_dependency->next = m_ready_tasks.load(std::memory_order_relaxed);
            while (!m_ready_tasks.compare_exchange_weak(task_dependency->next, task_dependency, std::memory_order_release, std::memory_order_relaxed));
        }

        task_dependency = next_task_dependency;
    }
}

} // namespace kw
