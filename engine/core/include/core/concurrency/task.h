#pragma once

#include <atomic>
#include <initializer_list>

namespace kw {

class MemoryResource;
class TaskNode;

class Task {
public:
    Task();
    
    // This task must run after given tasks. Calling this method is allowed before this task is enqueued or when it still
    // has other dependencies. Otherwise data race might happen and this task will run too early won't run at all.
    void add_input_dependencies(MemoryResource& transient_memory_resource, Task* const* input_dependencies, size_t input_dependency_count);

    // Shortcut for the previous method.
    void add_input_dependencies(MemoryResource& transient_memory_resource, std::initializer_list<Task*> input_dependencies);
    
    // This task must run before given tasks. Calling this method is allowed before the given tasks are enqueued or when they still
    // have other dependencies. Otherwise data race might happen and some of these tasks will run too early won't run at all.
    void add_output_dependencies(MemoryResource& transient_memory_resource, Task* const* output_dependencies, size_t output_dependency_count);

    // Shortcut for the previous method.
    void add_output_dependencies(MemoryResource& transient_memory_resource, std::initializer_list<Task*> output_dependencies);

    // Must be overriden by the user.
    virtual void run() = 0;

    // Could be overriden by the user.
    virtual const char* get_name() const;

private:
    std::atomic<TaskNode*> m_output_dependencies;
    std::atomic<uint32_t> m_input_dependency_count;

    friend class TaskScheduler;
};

// Task that doesn't do anything. Helps to manage complex dependencies. For example let's say we have TaskA and TaskB.
// TaskA produces some data that TaskB uses, so TaskB must run after TaskA. TaskA on the other hand produces its data
// in parallel and in order to do that enqueues TaskC1, TaskC2 and TaskC3. We don't want TaskB to know about these
// tasks because this is an implementation detail. What we can do is to say that TaskA consists of two tasks: TaskA1
// and no-op TaskA2. No-op TaskA2 depends on TaskA1 and therefore doesn't run until the latter has completed. TaskB
// depends on no-op TaskA2 and therefore doesn't run until both TaskA1 and no-op TaskA2 have completed. TaskA1 enqueues
// TaskC1, TaskC2 and TaskC3 and adds those as dependencies to TaskA2 (which is allowed, because TaskA1 has not yet
// completed and TaskA2 is dependent on it). Then when TaskC1, TaskC2 and TaskC3 are completed the no-op TaskA2
// completes and TaskB starts running.
class NoopTask : public Task {
public:
    NoopTask();
    NoopTask(const char* name);

    void run() override;

    const char* get_name() const override;

private:
    const char* m_name;
};

} // namespace kw
