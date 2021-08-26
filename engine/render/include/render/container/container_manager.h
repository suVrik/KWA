#pragma once

#include "render/container/container_prototype_notifier.h"

#include <core/containers/pair.h>
#include <core/containers/shared_ptr.h>
#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>

#include <shared_mutex>

namespace kw {

class AnimationManager;
class ContainerPrototype;
class GeometryManager;
class MaterialManager;
class ParticleSystemManager;
class Task;
class TaskScheduler;
class TextureManager;

struct ContainerManagerDescriptor {
    TaskScheduler* task_scheduler;
    TextureManager* texture_manager;
    GeometryManager* geometry_manager;
    MaterialManager* material_manager;
    AnimationManager* animation_manager;
    ParticleSystemManager* particle_system_manager;

    MemoryResource* persistent_memory_resource;
    MemoryResource* transient_memory_resource;
};

class ContainerManager {
public:
    explicit ContainerManager(const ContainerManagerDescriptor& descriptor);
    ~ContainerManager();

    // Enqueue container prototype loading if it's not yet loaded. Concurrent loads are allowed.
    SharedPtr<ContainerPrototype> load(const char* relative_path);

    // O(n) where n is the total number of loaded container prototypes. Designed for tools.
    const String& get_relative_path(const SharedPtr<ContainerPrototype>& particle_system) const;

    // The first task creates worker tasks that load all enqueued container prototypes at the moment. Those tasks
    // will be finished before the second task starts. If you are planning to load container prototypes on this frame,
    // you need to place your task before the first task. If you are planning to use container prototypes loaded on
    // this frame, you need to place your task after the second task.
    Pair<Task*, Task*> create_tasks();

private:
    class BeginTask;
    class WorkerTask;

    TaskScheduler& m_task_scheduler;
    TextureManager& m_texture_manager;
    GeometryManager& m_geometry_manager;
    MaterialManager& m_material_manager;
    AnimationManager& m_animation_manager;
    ParticleSystemManager& m_particle_system_manager;

    MemoryResource& m_persistent_memory_resource;
    MemoryResource& m_transient_memory_resource;

    UnorderedMap<String, SharedPtr<ContainerPrototype>> m_container_prototypes;
    Vector<Pair<const String&, SharedPtr<ContainerPrototype>>> m_pending_container_prototypes;
    mutable std::shared_mutex m_container_prototypes_mutex;

    ContainerPrototypeNotifier m_container_prototype_notifier;
};

} // namespace kw
