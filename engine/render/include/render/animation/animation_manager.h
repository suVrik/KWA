#pragma once

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class Animation;
class Task;
class TaskScheduler;

struct AnimationManagerDescriptor {
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class AnimationManager {
public:
    explicit AnimationManager(const AnimationManagerDescriptor& descriptor);
    ~AnimationManager();

    // Enqueue animation loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<Animation> load(const char* relative_path);

    // O(n) where n is the total number of loaded animations. Designed for tools.
    const String& get_relative_path(const SharedPtr<Animation>& animation) const;

    // The first task creates worker tasks that load all enqueued animations at the moment. Those tasks will be finished
    // before the second task starts. If you are planning to load animations on this frame, you need to place your task
    // before the first task. If you are planning to use animations loaded on this frame, you need to place your task
    // after the second task.
    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class PendingTask;

    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<Animation>> m_animations;
    Vector<Pair<const String&, SharedPtr<Animation>>> m_pending_animations;
    std::shared_mutex m_animations_mutex;
};

} // namespace kw
