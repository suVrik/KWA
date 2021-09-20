#include "core/prefab/prefab_manager.h"
#include "core/concurrency/task.h"
#include "core/concurrency/task_scheduler.h"
#include "core/debug/assert.h"
#include "core/error.h"
#include "core/io/markdown_reader.h"
#include "core/prefab/prefab_prototype.h"
#include "core/scene/primitive.h"
#include "core/scene/primitive_reflection.h"

namespace kw {

class PrefabManager::WorkerTask : public Task {
public:
    WorkerTask(PrefabManager& manager, PrefabPrototype& prefab_prototype, const String& relative_path)
        : m_manager(manager)
        , m_prefab_prototype(prefab_prototype)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        MarkdownReader reader(m_manager.m_transient_memory_resource, m_relative_path.c_str());

        Vector<UniquePtr<Primitive>> primitives(reader.get_size(), m_manager.m_persistent_memory_resource);
        for (size_t i = 0; i < reader.get_size(); i++) {
            KW_ASSERT(m_manager.m_primitive_reflection != nullptr, "Primitive reflection is not set.");
            primitives[i] = m_manager.m_primitive_reflection->create_from_markdown(reader[i].as<ObjectNode>());
        }
        m_prefab_prototype = PrefabPrototype(m_manager.m_prefab_prototype_notifier, std::move(primitives));

        m_manager.m_prefab_prototype_notifier.notify(m_prefab_prototype);
    }

    const char* get_name() const override {
        return "Prefab Manager Worker";
    }

private:
    PrefabManager& m_manager;
    PrefabPrototype& m_prefab_prototype;
    const String& m_relative_path;
};

class PrefabManager::BeginTask : public Task {
public:
    BeginTask(PrefabManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load prefab prototypes are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_prefab_prototypes_mutex);

        //
        // Destroy prefab prototypes that only referenced from `PrefabManager`.
        //

        for (auto it = m_manager.m_prefab_prototypes.begin(); it != m_manager.m_prefab_prototypes.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_prefab_prototypes.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new prefab prototypes.
        //

        for (auto& [relative_path, prefab_prototype] : m_manager.m_pending_prefab_prototypes) {
            WorkerTask* prefab_prototype_task = m_manager.m_transient_memory_resource.construct<WorkerTask>(m_manager, *prefab_prototype, relative_path);
            KW_ASSERT(prefab_prototype_task != nullptr);

            prefab_prototype_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, prefab_prototype_task);
        }

        m_manager.m_pending_prefab_prototypes.clear();
    }

    const char* get_name() const override {
        return "Prefab Manager Begin";
    }

private:
    PrefabManager& m_manager;
    Task* m_end_task;
};

PrefabManager::PrefabManager(const PrefabManagerDescriptor& descriptor)
    : m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_primitive_reflection(nullptr)
    , m_prefab_prototypes(*descriptor.persistent_memory_resource)
    , m_pending_prefab_prototypes(*descriptor.persistent_memory_resource)
    , m_prefab_prototype_notifier(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_prefab_prototypes.reserve(16);
    m_pending_prefab_prototypes.reserve(16);
}

PrefabManager::~PrefabManager() {
#ifdef KW_DEBUG
    m_pending_prefab_prototypes.clear();

    while (!m_prefab_prototypes.empty()) {
        bool any_erased = false;

        for (auto it = m_prefab_prototypes.begin(); it != m_prefab_prototypes.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_prefab_prototypes.erase(it);
                any_erased = true;
            } else {
                ++it;
            }
        }

        if (!any_erased) {
            KW_ASSERT(false, "Not all prefab prototypes are released.");
            break;
        }
    }
#endif // KW_DEBUG
}

void PrefabManager::set_primitive_reflection(PrimitiveReflection& primitive_reflection) {
    KW_ASSERT(m_primitive_reflection == nullptr, "Primitive reflection is already set.");
    m_primitive_reflection = &primitive_reflection;
}

SharedPtr<PrefabPrototype> PrefabManager::load(const char* relative_path) {
    {
        std::shared_lock shared_lock(m_prefab_prototypes_mutex);

        auto it = m_prefab_prototypes.find(String(relative_path, m_transient_memory_resource));
        if (it != m_prefab_prototypes.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_prefab_prototypes_mutex);

        auto [it, success] = m_prefab_prototypes.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<PrefabPrototype>());
        if (!success) {
            // Could return here if prefab prototype is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<PrefabPrototype>(m_persistent_memory_resource, m_prefab_prototype_notifier, m_persistent_memory_resource);

        m_pending_prefab_prototypes.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& PrefabManager::get_relative_path(const SharedPtr<PrefabPrototype>& prefab_prototype) const {
    std::shared_lock shared_lock(m_prefab_prototypes_mutex);

    for (auto& [relative_path, stored_prefab_prototype] : m_prefab_prototypes) {
        if (prefab_prototype == stored_prefab_prototype) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> PrefabManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Prefab Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
