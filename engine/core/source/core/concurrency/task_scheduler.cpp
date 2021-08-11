#include "core/concurrency/task_scheduler.h"
#include "core/concurrency/concurrency_utils.h"
#include "core/concurrency/task.h"
#include "core/concurrency/task_node.h"
#include "core/debug/assert.h"
#include "core/debug/cpu_profiler.h"

namespace kw {

TaskScheduler::TaskScheduler(MemoryResource& persistent_memory_resource, size_t thread_count)
    : m_ready_tasks(nullptr)
    , m_busy_threads(0)
    , m_is_running(true)
    , m_threads(persistent_memory_resource)
{
    m_threads.reserve(thread_count);
    for (size_t i = 0; i < thread_count; i++) {
        m_threads.push_back(std::thread(&TaskScheduler::worker_thread, this, i));
    }
}

TaskScheduler::~TaskScheduler() {
    // Gracefully terminate all worker threads.

    {
        std::lock_guard lock(m_mutex);
        m_is_running = false;
        m_ready_task_condition_variable.notify_all();
    }

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

        {
            std::lock_guard lock(m_mutex);
            new_head->next = m_ready_tasks;
            m_ready_tasks = new_head;
            m_ready_task_condition_variable.notify_one();
        }
    }
}

void TaskScheduler::join() {
    while (true) {
        std::unique_lock lock(m_mutex);

        if (m_ready_tasks != nullptr) {
            // Ready task is available, pop it from stack.
            TaskNode* head = m_ready_tasks;
            m_ready_tasks = head->next;

            // Run task in parallel.
            lock.unlock();
            run_task(head->task);
        } else if (m_busy_threads > 0) {
            // No ready tasks are available, but there're busy worker threads that might add them.
            m_busy_thread_condition_variable.wait(lock);
        } else {
            // No ready tasks available, all worker threads are idle, so no new tasks will be produced.
            return;
        }
    }
}

size_t TaskScheduler::get_thread_count() const {
    return m_threads.size();
}

void TaskScheduler::worker_thread(size_t thread_index) {
    // Other than running a task and waiting on condition variable, worker thread is locking the mutex.
    std::unique_lock lock(m_mutex);

    {
        char name_buffer[24];
        sprintf_s(name_buffer, sizeof(name_buffer), "Worker Thread %zu", thread_index);
        ConcurrencyUtils::set_current_thread_name(name_buffer);
    }

    while (m_is_running) {
        while (m_ready_tasks == nullptr) {
            // Wait for new ready tasks (or when `m_is_running` becomes false).
            m_ready_task_condition_variable.wait(lock);

            // Scheduler's destructor notifies `m_ready_task_condition_variable` when `m_is_running` is set to false.
            if (!m_is_running) {
                return;
            }
        }

        // Ready task is available, pop it from stack.
        TaskNode* head = m_ready_tasks;
        m_ready_tasks = head->next;

        // Run task in parallel.
        m_busy_threads++;
        lock.unlock();
        run_task(head->task);
        lock.lock();
        m_busy_threads--;

        // Main thread might be waiting for worker threads to finish.
        m_busy_thread_condition_variable.notify_one();
    }
}

void TaskScheduler::run_task(Task* task) {
    // Run task.

    {
        KW_CPU_PROFILER(task->get_name());

        task->run();
    }

    // Notify future dependencies that this task has completed.

    TaskNode* task_dependency = task->m_output_dependencies.load(std::memory_order_relaxed);
    while (!task->m_output_dependencies.compare_exchange_weak(task_dependency, &INVALID_TASK_NODE, std::memory_order_release, std::memory_order_relaxed));

    // Notify past dependencies that this task has completed.

    while (task_dependency != nullptr) {
        // We may reuse `task_dependency` for ready stack and overwrite `next`.
        TaskNode* next_task_dependency = task_dependency->next;

        size_t input_dependency_count = --task_dependency->task->m_input_dependency_count;
        if (input_dependency_count == 0) {
            // Task doesn't have any dependencies left. Move it from dependency stack to ready stack.
            std::lock_guard lock(m_mutex);
            task_dependency->next = m_ready_tasks;
            m_ready_tasks = task_dependency;
            m_ready_task_condition_variable.notify_one();
        }

        task_dependency = next_task_dependency;
    }
}

} // namespace kw
