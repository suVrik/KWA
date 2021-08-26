#include "render/container/container_manager.h"
#include "render/container/container_prototype.h"
#include "render/scene/primitive.h"
#include "render/scene/primitive_reflection.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown_reader.h>

namespace kw {

class ContainerManager::WorkerTask : public Task {
public:
    WorkerTask(ContainerManager& manager, ContainerPrototype& container_prototype, const String& relative_path)
        : m_manager(manager)
        , m_container_prototype(container_prototype)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path.c_str());

        PrimitiveReflection& reflection = PrimitiveReflection::instance();

        PrimitiveReflectionDescriptor primitive_reflection_descriptor{};
        primitive_reflection_descriptor.texture_manager = &m_manager.m_texture_manager;
        primitive_reflection_descriptor.geometry_manager = &m_manager.m_geometry_manager;
        primitive_reflection_descriptor.material_manager = &m_manager.m_material_manager;
        primitive_reflection_descriptor.animation_manager = &m_manager.m_animation_manager;
        primitive_reflection_descriptor.particle_system_manager = &m_manager.m_particle_system_manager;
        primitive_reflection_descriptor.container_manager = &m_manager;
        primitive_reflection_descriptor.persistent_memory_resource = &m_manager.m_persistent_memory_resource;

        Vector<UniquePtr<Primitive>> primitives(reader.get_size(), m_manager.m_persistent_memory_resource);

        for (size_t i = 0; i < reader.get_size(); i++) {
            primitive_reflection_descriptor.primitive_node = &reader[i].as<ObjectNode>();

            primitives[i] = reflection.create_from_markdown(primitive_reflection_descriptor);
        }

        m_container_prototype = ContainerPrototype(m_manager.m_container_prototype_notifier, std::move(primitives));

        m_manager.m_container_prototype_notifier.notify(m_container_prototype);
    }

    const char* get_name() const override {
        return "Container Manager Worker";
    }

private:
    ContainerManager& m_manager;
    ContainerPrototype& m_container_prototype;
    const String& m_relative_path;
};

class ContainerManager::BeginTask : public Task {
public:
    BeginTask(ContainerManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load container prototypes are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_container_prototypes_mutex);

        //
        // Destroy container prototypes that only referenced from `ContainerManager`.
        //

        for (auto it = m_manager.m_container_prototypes.begin(); it != m_manager.m_container_prototypes.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_container_prototypes.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new container prototypes.
        //

        for (auto& [relative_path, container_prototype] : m_manager.m_pending_container_prototypes) {
            WorkerTask* container_prototype_task = m_manager.m_transient_memory_resource.construct<WorkerTask>(m_manager, *container_prototype, relative_path);
            KW_ASSERT(container_prototype_task != nullptr);

            container_prototype_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, container_prototype_task);
        }

        m_manager.m_pending_container_prototypes.clear();
    }

    const char* get_name() const override {
        return "Container Manager Begin";
    }

private:
    ContainerManager& m_manager;
    Task* m_end_task;
};

ContainerManager::ContainerManager(const ContainerManagerDescriptor& descriptor)
    : m_task_scheduler(*descriptor.task_scheduler)
    , m_texture_manager(*descriptor.texture_manager)
    , m_geometry_manager(*descriptor.geometry_manager)
    , m_material_manager(*descriptor.material_manager)
    , m_animation_manager(*descriptor.animation_manager)
    , m_particle_system_manager(*descriptor.particle_system_manager)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_container_prototypes(*descriptor.persistent_memory_resource)
    , m_pending_container_prototypes(*descriptor.persistent_memory_resource)
    , m_container_prototype_notifier(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.texture_manager != nullptr, "Invalid texture manager.");
    KW_ASSERT(descriptor.geometry_manager != nullptr, "Invalid geometry manager.");
    KW_ASSERT(descriptor.material_manager != nullptr, "Invalid material manager.");
    KW_ASSERT(descriptor.animation_manager != nullptr, "Invalid animation manager.");
    KW_ASSERT(descriptor.particle_system_manager != nullptr, "Invalid particle system manager.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_container_prototypes.reserve(16);
    m_pending_container_prototypes.reserve(16);
}

ContainerManager::~ContainerManager() = default;

SharedPtr<ContainerPrototype> ContainerManager::load(const char* relative_path) {
    {
        std::shared_lock shared_lock(m_container_prototypes_mutex);

        auto it = m_container_prototypes.find(String(relative_path, m_transient_memory_resource));
        if (it != m_container_prototypes.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_container_prototypes_mutex);

        auto [it, success] = m_container_prototypes.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<ContainerPrototype>());
        if (!success) {
            // Could return here if container prototypes is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<ContainerPrototype>(m_persistent_memory_resource, m_container_prototype_notifier, m_persistent_memory_resource);

        m_pending_container_prototypes.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& ContainerManager::get_relative_path(const SharedPtr<ContainerPrototype>& container_prototype) const {
    std::shared_lock shared_lock(m_container_prototypes_mutex);

    for (auto& [relative_path, stored_container_prototype] : m_container_prototypes) {
        if (container_prototype == stored_container_prototype) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> ContainerManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Container Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
