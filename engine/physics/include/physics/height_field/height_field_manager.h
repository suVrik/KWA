#pragma once

#include "physics/height_field/height_field_notifier.h"

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class HeightField;
class PhysicsManager;
class Task;
class TaskScheduler;

struct HeightFieldManagerDescriptor {
    PhysicsManager* physics_manager;
    TaskScheduler* task_scheduler;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class HeightFieldManager {
public:
    explicit HeightFieldManager(const HeightFieldManagerDescriptor& descriptor);
    ~HeightFieldManager();

    SharedPtr<HeightField> load(const char* height_field_path);

    // O(n) where n is the total number of loaded height fields. Designed for tools.
    const String& get_relative_path(const SharedPtr<HeightField>& height_field) const;

    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    PhysicsManager& m_physics_manager;
    TaskScheduler& m_task_scheduler;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<HeightField>> m_height_fields;
    Vector<Pair<const String&, SharedPtr<HeightField>>> m_pending_height_fields;
    mutable std::shared_mutex m_height_fields_mutex;

    HeightFieldNotifier m_height_field_notifier;
};

} // namespace kw
