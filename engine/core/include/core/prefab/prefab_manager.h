#pragma once

#include "core/containers/pair.h"
#include "core/containers/shared_ptr.h"
#include "core/containers/string.h"
#include "core/containers/unordered_map.h"
#include "core/containers/vector.h"
#include "core/prefab/prefab_prototype_notifier.h"

#include <shared_mutex>

namespace kw {

class PrefabPrototype;
class PrimitiveReflection;
class Task;
class TaskScheduler;

struct PrefabManagerDescriptor {
    TaskScheduler* task_scheduler;
    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class PrefabManager {
public:
    explicit PrefabManager(const PrefabManagerDescriptor& descriptor);
    ~PrefabManager();

    // TODO: I don't like this circular dependency. I don't have any better idea though.
    void set_primitive_reflection(PrimitiveReflection& primitive_reflection);

    // Enqueue prefab prototype loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<PrefabPrototype> load(const char* relative_path);

    // O(n) where n is the total number of loaded prefab prototypes. Designed for tools.
    const String& get_relative_path(const SharedPtr<PrefabPrototype>& particle_system) const;

    // The first task creates worker tasks that load all enqueued prefab prototypes at the moment. Those tasks
    // will be finished before the second task starts. If you are planning to load prefab prototypes on this frame,
    // you need to place your task before the first task. If you are planning to use prefab prototypes loaded on
    // this frame, you need to place your task after the second task.
    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    TaskScheduler& m_task_scheduler;
    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    PrimitiveReflection* m_primitive_reflection;

    UnorderedMap<String, SharedPtr<PrefabPrototype>> m_prefab_prototypes;
    Vector<Pair<const String&, SharedPtr<PrefabPrototype>>> m_pending_prefab_prototypes;
    mutable std::shared_mutex m_prefab_prototypes_mutex;

    PrefabPrototypeNotifier m_prefab_prototype_notifier;
};

} // namespace kw
