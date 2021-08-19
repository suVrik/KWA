#pragma once

#include "render/particles/particle_system_notifier.h"

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class GeometryManager;
class MaterialManager;
class ParticleSystem;
class Task;
class TaskScheduler;

struct ParticleSystemManagerDescriptor {
    TaskScheduler* task_scheduler;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ParticleSystemManager {
public:
    explicit ParticleSystemManager(const ParticleSystemManagerDescriptor& descriptor);
    ~ParticleSystemManager();

    // Enqueue particle system loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<ParticleSystem> load(const char* relative_path);

    // O(n) where n is the total number of loaded particle systems. Designed for tools.
    const String& get_relative_path(const SharedPtr<ParticleSystem>& particle_system) const;

    // The first task creates worker tasks that load all enqueued particle systems at the moment. Those tasks will be
    // finished before the second task starts. If you are planning to load particle systems on this frame, you need to
    // place your task before the first task. If you are planning to use particle systems loaded on this frame,
    // you need to place your task after the second task.
    Pair<Task*, Task*> create_tasks();

    // TODO: Make it private.
    ParticleSystemNotifier m_particle_system_notifier;

private:
    class BeginTask;
    class WorkerTask;

    TaskScheduler& m_task_scheduler;
    GeometryManager& m_geometry_manager;
    MaterialManager& m_material_manager;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<ParticleSystem>> m_particle_systems;
    Vector<Pair<const String&, SharedPtr<ParticleSystem>>> m_pending_particle_systems;
    mutable std::shared_mutex m_particle_systems_mutex;
};

} // namespace kw
