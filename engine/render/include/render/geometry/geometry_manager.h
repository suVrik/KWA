#pragma once

#include "render/geometry/geometry_notifier.h"

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class Geometry;
class Render;
class Task;
class TaskScheduler;

struct GeometryManagerDescriptor {
    Render* render;
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class GeometryManager {
public:
    explicit GeometryManager(const GeometryManagerDescriptor& descriptor);
    ~GeometryManager();

    // Enqueue geometry loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<Geometry> load(const char* relative_path);

    // O(n) where n is the total number of loaded geometry. Designed for tools.
    const String& get_relative_path(const SharedPtr<Geometry>& geometry) const;

    // The first task creates worker tasks that load all enqueued geometry at the moment. Those tasks will be finished
    // before the second task starts. If you are planning to load geometry on this frame, you need to place your task
    // before the first task. If you are planning to use geometry loaded on this frame, you need to place your task
    // after the second task.
    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    Render& m_render;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<Geometry>> m_geometry;
    Vector<Pair<const String&, SharedPtr<Geometry>>> m_pending_geometry;
    mutable std::shared_mutex m_geometry_mutex;

    GeometryNotifier m_geometry_notifier;
};

} // namespace kw
