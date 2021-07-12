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

void Task::add_input_dependencies(MemoryResource& transient_memory_resource, Task* const* input_dependencies, size_t input_dependency_count) {
    KW_ASSERT(input_dependencies != nullptr);
    KW_ASSERT(input_dependency_count > 0);

    m_input_dependency_count += input_dependency_count;

    TaskNode* new_head = transient_memory_resource.allocate<TaskNode>(input_dependency_count);
    KW_ASSERT(new_head != nullptr);

    for (size_t i = 0; i < input_dependency_count; i++) {
        Task* input_dependency = input_dependencies[i];
        KW_ASSERT(input_dependency != nullptr);

        new_head->task = this;
        new_head->next = input_dependency->m_output_dependencies.load(std::memory_order_relaxed);

        do {
            // This is highly unlikely.
            if (new_head->next == &INVALID_TASK_NODE) {
                // This task has completed, so no need to wait for it. Input dependency count can't become zero because
                // this task must be either not enqueued yet or still have other dependencies.
                m_input_dependency_count--;
                break;
            }
        } while (!input_dependency->m_output_dependencies.compare_exchange_weak(new_head->next, new_head, std::memory_order_release, std::memory_order_relaxed));

        new_head++;
    }
}

void Task::add_input_dependencies(MemoryResource& transient_memory_resource, std::initializer_list<Task*> input_dependencies) {
    return add_input_dependencies(transient_memory_resource, input_dependencies.begin(), input_dependencies.size());
}

void Task::add_output_dependencies(MemoryResource& transient_memory_resource, Task* const* output_dependencies, size_t output_dependency_count) {
    KW_ASSERT(output_dependencies != nullptr);
    KW_ASSERT(output_dependency_count > 0);

    TaskNode* new_head = transient_memory_resource.allocate<TaskNode>(output_dependency_count);
    KW_ASSERT(new_head != nullptr);

    size_t i;
    for (i = 0; i + 1 < output_dependency_count; i++) {
        KW_ASSERT(output_dependencies[i] != nullptr);
        output_dependencies[i]->m_input_dependency_count++;

        new_head->task = output_dependencies[i];
        new_head->next = new_head + 1;

        new_head++;
    }

    KW_ASSERT(output_dependencies[i] != nullptr);
    output_dependencies[i]->m_input_dependency_count++;

    new_head->task = output_dependencies[i];
    new_head->next = m_output_dependencies.load(std::memory_order_relaxed);

    do {
        // This is highly unlikely.
        if (new_head->next == &INVALID_TASK_NODE) {
            // This task has completed, so no other task need to wait for it.
            for (size_t i = 0; i < output_dependency_count; i++) {
                // Input dependency count can't become zero because every dependency must be either not enqueued yet
                // or still have other dependencies.
                output_dependencies[i]->m_input_dependency_count--;
            }
            break;
        }
    } while (!m_output_dependencies.compare_exchange_weak(new_head->next, new_head, std::memory_order_release, std::memory_order_relaxed));
}

void Task::add_output_dependencies(MemoryResource& transient_memory_resource, std::initializer_list<Task*> output_dependencies) {
    return add_output_dependencies(transient_memory_resource, output_dependencies.begin(), output_dependencies.size());
}

const char* Task::get_name() const {
    return "Nameless Task";
}

NoopTask::NoopTask()
    : m_name("Nameless No-op Task")
{
}

NoopTask::NoopTask(const char* name)
    : m_name(name)
{
}

void NoopTask::run() {
    // No-op.
}

const char* NoopTask::get_name() const {
    return m_name;
}

} // namespace kw
