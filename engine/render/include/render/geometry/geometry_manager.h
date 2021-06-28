#pragma once

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

    // The first task creates worker tasks that load all enqueued geometries at the moment. Those tasks will be finished
    // before the second task starts. If you are planning to load geometries on this frame, you need to place your task
    // before the first task. If you are planning to use geometries loaded on this frame, you need to place your task
    // after the second task.
    std::pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class PendingTask;

    Render& m_render;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<Geometry>> m_geometries;
    std::shared_mutex m_geometries_mutex;

    // Geometries that are not even opened yet.
    Vector<std::pair<const String&, SharedPtr<Geometry>>> m_pending_geometries;
};

} // namespace kw
