#pragma once

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class BlendTreeManager;
class MotionGraph;
class Task;
class TaskScheduler;

struct MotionGraphManagerDescriptor {
    BlendTreeManager* blend_tree_manager;
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class MotionGraphManager {
public:
    explicit MotionGraphManager(const MotionGraphManagerDescriptor& descriptor);
    ~MotionGraphManager();

    SharedPtr<MotionGraph> load(const char* relative_path);

    // O(n) where n is the total number of loaded motion graphs. Designed for tools.
    const String& get_relative_path(const SharedPtr<MotionGraph>& blend_tree) const;

    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class PendingTask;

    BlendTreeManager& m_blend_tree_manager;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<MotionGraph>> m_motion_graphs;
    Vector<Pair<const String&, SharedPtr<MotionGraph>>> m_pending_motion_graphs;
    std::shared_mutex m_motion_graphs_mutex;
};

} // namespace kw
