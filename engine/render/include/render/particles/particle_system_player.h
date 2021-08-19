#pragma once

#include <core/containers/pair.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class ParticleSystemPrimitive;
class Task;
class TaskScheduler;
class Timer;

struct ParticleSystemPlayerDescriptor {
    Timer* timer;
    TaskScheduler* task_scheduler;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ParticleSystemPlayer {
public:
    explicit ParticleSystemPlayer(const ParticleSystemPlayerDescriptor& descriptor);

    void add(ParticleSystemPrimitive& primitive);
    void remove(ParticleSystemPrimitive& primitive);

    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    Timer& m_timer;
    TaskScheduler& m_task_scheduler;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    Vector<ParticleSystemPrimitive*> m_primitives;
    std::shared_mutex m_primitives_mutex;
};

} // namespace kw
