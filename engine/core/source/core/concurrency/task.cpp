#include "core/concurrency/task.h"
#include "core/concurrency/task_node.h"
#include "core/debug/assert.h"
#include "core/memory/memory_resource.h"

namespace kw {

// Dependency count is 1 by default to prevent task from running before task is enqueued in task scheduler.
Task::Task()
    : m_output_dependencies(nullptr)
    , m_input_dependency_count(1)
{
}

void Task::add_dependencies(MemoryResource& transient_memory_resource, Task** input_dependencies, size_t input_dependency_count) {
    m_input_dependency_count += input_dependency_count;

    TaskNode* new_head = transient_memory_resource.allocate<TaskNode>(input_dependency_count);
    for (size_t i = 0; i < input_dependency_count; i++) {
        Task* input_dependency = input_dependencies[i];
        KW_ASSERT(input_dependency != nullptr);

        new_head->task = this;
        new_head->next = input_dependency->m_output_dependencies.load(std::memory_order_relaxed);

        do {
            if (new_head->next == &INVALID_TASK_NODE) {
                // This task has completed, so no need to wait for it.
                m_input_dependency_count--;
                break;
            }
        } while (!input_dependency->m_output_dependencies.compare_exchange_weak(new_head->next, new_head, std::memory_order_release, std::memory_order_relaxed));

        new_head++;
    }
}

void NoopTask::run() {
}

} // namespace kw
