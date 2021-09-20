#include "physics/height_field/height_field_manager.h"
#include "physics/height_field/height_field.h"
#include "physics/physics_manager.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/memory/malloc_memory_resource.h>

#include <PxPhysics.h>
#include <cooking/PxCooking.h>
#include <geometry/PxHeightField.h>
#include <geometry/PxHeightFieldDesc.h>
#include <geometry/PxHeightFieldSample.h>

#include <algorithm>
#include <fstream>

namespace kw {

class HeightFieldManager::WorkerTask : public Task {
public:
    WorkerTask(HeightFieldManager& manager, HeightField& height_field, const char* relative_path)
        : m_manager(manager)
        , m_height_fields(height_field)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        // TODO: Perhaps use markdown instead of TSV for consistency with other resources?
        //   [
        //     [ 0, 0, 0, 1 ],
        //     [ 0, 1, 0, 0 ],
        //     [ 0, 0, 1, 0 ],
        //     [ 1, 1, 0, 1 ]
        //   ]
        std::ifstream stream(m_relative_path);
        KW_ERROR(stream, "Failed to open height field \"%s\".", m_relative_path);

        size_t rows, columns;
        KW_ERROR((stream >> rows >> columns), "Failed to read height field header \"%s\".", m_relative_path);

        Vector<physx::PxHeightFieldSample> samples(rows * columns, physx::PxHeightFieldSample(), m_manager.m_transient_memory_resource);

        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < columns; j++) {
                float sample;
                KW_ERROR((stream >> sample), "Failed to read height field sample \"%s\".", m_relative_path);

                samples[i * columns + j].height = std::clamp(static_cast<int32_t>(sample * INT16_MAX), -32768, 32767);
            }
        }

        physx::PxHeightFieldDesc height_field_descriptor;
        height_field_descriptor.format = physx::PxHeightFieldFormat::eS16_TM;
        height_field_descriptor.nbColumns = static_cast<uint32_t>(columns);
        height_field_descriptor.nbRows = static_cast<uint32_t>(rows);
        height_field_descriptor.samples.data = samples.data();
        height_field_descriptor.samples.stride = sizeof(physx::PxHeightFieldSample);

        PhysicsPtr<physx::PxHeightField> height_field = m_manager.m_physics_manager.get_cooking().createHeightField(
            height_field_descriptor, m_manager.m_physics_manager.get_physics().getPhysicsInsertionCallback()
        );

        m_height_fields = HeightField(m_manager.m_height_field_notifier, std::move(height_field));

        m_manager.m_height_field_notifier.notify(m_height_fields);
    }

    const char* get_name() const override {
        return "Height Field Manager Worker";
    }

private:
    HeightFieldManager& m_manager;
    HeightField& m_height_fields;
    const char* m_relative_path;
};

class HeightFieldManager::BeginTask : public Task {
public:
    BeginTask(HeightFieldManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load height fields are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_height_fields_mutex);

        //
        // Destroy height fields that only referenced from `HeightFieldManager`.
        //

        for (auto it = m_manager.m_height_fields.begin(); it != m_manager.m_height_fields.end(); ) {
            if (it->second.use_count() == 1) {
                it = m_manager.m_height_fields.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new height fields.
        //

        for (auto& [relative_path, height_field] : m_manager.m_pending_height_fields) {
            WorkerTask* pending_task = m_manager.m_transient_memory_resource.construct<WorkerTask>(m_manager, *height_field, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_height_fields.clear();
    }

    const char* get_name() const override {
        return "Height Field Manager Begin";
    }

private:
    HeightFieldManager& m_manager;
    Task* m_end_task;
};

HeightFieldManager::HeightFieldManager(const HeightFieldManagerDescriptor& descriptor)
    : m_physics_manager(*descriptor.physics_manager)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_height_fields(*descriptor.persistent_memory_resource)
    , m_pending_height_fields(*descriptor.persistent_memory_resource)
    , m_height_field_notifier(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.physics_manager != nullptr, "Invalid physics manager.");
    KW_ASSERT(descriptor.task_scheduler != nullptr, "Invalid task scheduler.");
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr, "Invalid persistent memory resource.");
    KW_ASSERT(descriptor.transient_memory_resource != nullptr, "Invalid transient memory resource.");

    m_height_fields.reserve(32);
    m_pending_height_fields.reserve(32);
}

HeightFieldManager::~HeightFieldManager() {
    m_pending_height_fields.clear();

    for (auto& [relative_path, height_field] : m_height_fields) {
        KW_ASSERT(height_field.use_count() == 1, "Not all height fields are released.");
    }
}

SharedPtr<HeightField> HeightFieldManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_height_fields_mutex);

        auto it = m_height_fields.find(String(relative_path, m_transient_memory_resource));
        if (it != m_height_fields.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_height_fields_mutex);

        auto [it, success] = m_height_fields.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<HeightField>());
        if (!success) {
            // Could return here if height_field is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<HeightField>(m_persistent_memory_resource, m_height_field_notifier);

        m_pending_height_fields.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& HeightFieldManager::get_relative_path(const SharedPtr<HeightField>& height_field) const {
    std::shared_lock shared_lock(m_height_fields_mutex);

    for (auto& [relative_path, stored_height_field] : m_height_fields) {
        if (height_field == stored_height_field) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> HeightFieldManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Height Field Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
