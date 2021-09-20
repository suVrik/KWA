#pragma once

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class AnimationManager;
class BlendTree;
class Task;
class TaskScheduler;

struct BlendTreeManagerDescriptor {
    AnimationManager* animation_manager;
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class BlendTreeManager {
public:
    explicit BlendTreeManager(const BlendTreeManagerDescriptor& descriptor);
    ~BlendTreeManager();

    SharedPtr<BlendTree> load(const char* relative_path);

    // O(n) where n is the total number of loaded blend trees. Designed for tools.
    const String& get_relative_path(const SharedPtr<BlendTree>& blend_tree) const;

    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class PendingTask;

    AnimationManager& m_animation_manager;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<BlendTree>> m_blend_trees;
    Vector<Pair<const String&, SharedPtr<BlendTree>>> m_pending_blend_trees;
    std::shared_mutex m_blend_trees_mutex;
};

} // namespace kw
