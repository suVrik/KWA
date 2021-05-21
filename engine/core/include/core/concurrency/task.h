#pragma once

#include <atomic>

namespace kw {

class MemoryResource;
class TaskNode;

class Task {
public:
    Task();
    
    // This task must run before given tasks. Calling this method makes sense before the task is enqueued or when it
    // has other dependencies. When a task ends up in ready stack, new input dependencies won't stop it from running.
    void add_dependencies(MemoryResource& transient_memory_resource, Task** input_dependencies, size_t input_dependency_count);

    // Must be overriden by the user.
    virtual void run() = 0;

private:
    std::atomic<TaskNode*> m_output_dependencies;
    std::atomic<uint32_t> m_input_dependency_count;

    friend class TaskScheduler;
};

// Task that doesn't do anything. Helps to manage complex dependencies. For example let's say we have TaskA and TaskB.
// TaskA produces some data that TaskB uses, so TaskB must run after TaskA. TaskA on the other hand produces its data
// in parallel and in order to do that enqueues TaskC1, TaskC2 and TaskC3. We don't want SystemB to know about these
// tasks because this is an implementation detail. What we can do is to say that TaskA consists of two tasks: TaskA1
// and no-op TaskA2. No-op TaskA2 depends on TaskA1 and therefore doesn't run until the latter has completed. TaskB
// depends on no-op TaskA2 and therefore doesn't run until both TaskA1 and no-op TaskA2 have completed. TaskA1 enqueues
// TaskC1, TaskC2 and TaskC3 and adds those as dependencies to TaskA2 (which is allowed, because TaskA1 has not yet
// completed and TaskA2 is dependent on it). Then when TaskC1, TaskC2 and TaskC3 are completed the no-op TaskA2
// completes and TaskB starts running.
class NoopTask : public Task {
public:
    void run() override;
};

} // namespace kw
