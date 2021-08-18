#pragma once

#include <core/containers/pair.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class AnimatedGeometryPrimitive;
class Task;
class TaskScheduler;
class Timer;

struct AnimationPlayerDescriptor {
    Timer* timer;
    TaskScheduler* task_scheduler;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class AnimationPlayer {
public:
    explicit AnimationPlayer(const AnimationPlayerDescriptor& descriptor);

    void add(AnimatedGeometryPrimitive& primitive);
    void remove(AnimatedGeometryPrimitive& primitive);
    
    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    Timer& m_timer;
    TaskScheduler& m_task_scheduler;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    Vector<AnimatedGeometryPrimitive*> m_primitives;
    std::shared_mutex m_primitives_mutex;
};

} // namespace kw
