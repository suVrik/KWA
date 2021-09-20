#include "render/particles/particle_system_manager.h"
#include "render/particles/particle_system.h"
#include "render/particles/particle_system_notifier.h"
#include "render/particles/particle_system_reflection.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown_reader.h>

namespace kw {

class ParticleSystemManager::WorkerTask : public Task {
public:
    WorkerTask(ParticleSystemManager& manager, ParticleSystem& particle_system, const String& relative_path)
        : m_manager(manager)
        , m_particle_system(particle_system)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path.c_str());

        ParticleSystemReflectionDescriptor reflection_descriptor{};
        reflection_descriptor.particle_system_node = &reader[0].as<ObjectNode>();
        reflection_descriptor.particle_system_notifier = &m_manager.m_particle_system_notifier;
        reflection_descriptor.geometry_manager = &m_manager.m_geometry_manager;
        reflection_descriptor.material_manager = &m_manager.m_material_manager;
        reflection_descriptor.persistent_memory_resource = &m_manager.m_persistent_memory_resource;

        m_particle_system = ParticleSystem(ParticleSystemReflection::instance().create_from_markdown(reflection_descriptor));

        m_manager.m_particle_system_notifier.notify(m_particle_system);
    }

    const char* get_name() const override {
        return "Particle System Manager Worker";
    }

private:
    ParticleSystemManager& m_manager;
    ParticleSystem& m_particle_system;
    const String& m_relative_path;
};

class ParticleSystemManager::BeginTask : public Task {
public:
    BeginTask(ParticleSystemManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load particle systems are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_particle_systems_mutex);

        //
        // Start loading brand new particle systems.
        //

        for (auto& [relative_path, particle_system] : m_manager.m_pending_particle_systems) {
            WorkerTask* particle_system_task = m_manager.m_transient_memory_resource.construct<WorkerTask>(m_manager, *particle_system, relative_path);
            KW_ASSERT(particle_system_task != nullptr);

            particle_system_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, particle_system_task);
        }

        m_manager.m_pending_particle_systems.clear();

        //
        // Destroy particle systems that only referenced from `ParticleSystemManager`.
        //

        for (auto it = m_manager.m_particle_systems.begin(); it != m_manager.m_particle_systems.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_particle_systems.erase(it);
            } else {
                ++it;
            }
        }
    }

    const char* get_name() const override {
        return "Particle System Manager Begin";
    }

private:
    ParticleSystemManager& m_manager;
    Task* m_end_task;
};

ParticleSystemManager::ParticleSystemManager(const ParticleSystemManagerDescriptor& descriptor)
    : m_task_scheduler(*descriptor.task_scheduler)
    , m_geometry_manager(*descriptor.geometry_manager)
    , m_material_manager(*descriptor.material_manager)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_particle_systems(*descriptor.persistent_memory_resource)
    , m_pending_particle_systems(*descriptor.persistent_memory_resource)
    , m_particle_system_notifier(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.geometry_manager != nullptr, "Invalid geometry manager.");
    KW_ASSERT(descriptor.material_manager != nullptr, "Invalid material manager.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_particle_systems.reserve(16);
    m_pending_particle_systems.reserve(16);
}

ParticleSystemManager::~ParticleSystemManager() {
    m_pending_particle_systems.clear();

    for (auto& [_, particle_system] : m_particle_systems) {
        KW_ASSERT(particle_system.use_count() == 1, "Not all particle systems are released.");
    }
    m_particle_systems.clear();
}

SharedPtr<ParticleSystem> ParticleSystemManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_particle_systems_mutex);

        auto it = m_particle_systems.find(String(relative_path, m_transient_memory_resource));
        if (it != m_particle_systems.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_particle_systems_mutex);

        auto [it, success] = m_particle_systems.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<ParticleSystem>());
        if (!success) {
            // Could return here if particle systems is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<ParticleSystem>(m_persistent_memory_resource, m_particle_system_notifier);

        m_pending_particle_systems.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& ParticleSystemManager::get_relative_path(const SharedPtr<ParticleSystem>& particle_system) const {
    std::shared_lock shared_lock(m_particle_systems_mutex);

    for (auto& [relative_path, stored_particle_system] : m_particle_systems) {
        if (particle_system == stored_particle_system) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> ParticleSystemManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Particle System Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
