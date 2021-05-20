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

} // namespace kw
