#include "render/geometry/geometry_manager.h"
#include "render/geometry/geometry.h"
#include "render/render.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/math/float4.h>
#include <core/utils/parser_utils.h>

namespace kw {

namespace EndianUtils {

float2 swap_le(float2 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    return vector;
}

float3 swap_le(float3 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    return vector;
}

float4 swap_le(float4 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    vector.w = swap_le(vector.w);
    return vector;
}

Geometry::Vertex swap_le(Geometry::Vertex vertex) {
    vertex.position = swap_le(vertex.position);
    vertex.normal = swap_le(vertex.normal);
    vertex.tangent = swap_le(vertex.tangent);
    vertex.texcoord_0 = swap_le(vertex.texcoord_0);
    return vertex;
}

} // namespace EndianUtils

constexpr uint32_t KWG_SIGNATURE = ' GWK';

class GeometryManager::PendingTask final : public Task {
public:
    PendingTask(GeometryManager& manager, Geometry& geometry, const char* relative_path)
        : m_manager(manager)
        , m_geometry(geometry)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        Reader reader(m_relative_path);
        KW_ERROR(reader, "Failed to open geometry \"%s\".", m_relative_path);
        KW_ERROR(read_next(reader) == KWG_SIGNATURE, "Invalid geometry \"%s\" signature.", m_relative_path);

        uint32_t vertex_count = read_next(reader);
        uint32_t index_count = read_next(reader);

        aabbox bounds;
        KW_ERROR(reader.read_le<float>(bounds.data, std::size(bounds.data)), "Failed to read geometry header.");

        Vector<Geometry::Vertex> vertices(vertex_count, m_manager.m_transient_memory_resource);
        KW_ERROR(reader.read_le<Geometry::Vertex>(vertices.data(), vertices.size()), "Failed to read geometry vertices.");

        VertexBuffer* vertex_buffer = m_manager.m_render.create_vertex_buffer(m_relative_path, sizeof(Geometry::Vertex) * vertices.size());
        KW_ASSERT(vertex_buffer != nullptr);

        m_manager.m_render.upload_vertex_buffer(vertex_buffer, vertices.data(), sizeof(Geometry::Vertex) * vertices.size());

        IndexBuffer* index_buffer;

        if (vertex_count < UINT16_MAX) {
            Vector<uint16_t> indices(index_count, m_manager.m_transient_memory_resource);
            KW_ERROR(reader.read_le<uint16_t>(indices.data(), indices.size()), "Failed to read geometry indices.");

            index_buffer = m_manager.m_render.create_index_buffer(m_relative_path, sizeof(uint16_t) * indices.size(), IndexSize::UINT16);
            KW_ASSERT(index_buffer != nullptr);

            m_manager.m_render.upload_index_buffer(index_buffer, indices.data(), sizeof(uint16_t) * indices.size());
        } else {
            Vector<uint32_t> indices(index_count, m_manager.m_transient_memory_resource);
            KW_ERROR(reader.read_le<uint32_t>(indices.data(), indices.size()), "Failed to read geometry indices.");

            index_buffer = m_manager.m_render.create_index_buffer(m_relative_path, sizeof(uint32_t) * index_count, IndexSize::UINT32);
            KW_ASSERT(index_buffer != nullptr);

            m_manager.m_render.upload_index_buffer(index_buffer, indices.data(), sizeof(uint32_t) * indices.size());
        }

        m_geometry = Geometry(vertex_buffer, index_buffer, index_count, bounds);
    }

    const char* get_name() const override {
        return "Geometry Manager Pending";
    }

private:
    uint32_t read_next(Reader& reader) {
        std::optional<uint32_t> value = reader.read_le<uint32_t>();
        KW_ERROR(value, "Failed to read geometry header.");
        return *value;
    }

    GeometryManager& m_manager;
    Geometry& m_geometry;
    const char* m_relative_path;
};

class GeometryManager::BeginTask final : public Task {
public:
    BeginTask(GeometryManager& manager, Task* end_task)
        : m_manager(manager)
        , m_end_task(end_task)
    {
    }

    void run() override {
        // Tasks that load geometries are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_geometries_mutex);

        //
        // Start loading brand new geometries.
        //

        for (auto& [relative_path, geometry] : m_manager.m_pending_geometries) {
            PendingTask* pending_task = new (m_manager.m_transient_memory_resource.allocate<PendingTask>()) PendingTask(m_manager, *geometry, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_geometries.clear();

        //
        // Destroy geometries that only referenced from `GeometryManager`.
        //

        for (auto it = m_manager.m_geometries.begin(); it != m_manager.m_geometries.end(); ) {
            if (it->second.use_count() == 1) {
                m_manager.m_render.destroy_vertex_buffer(it->second->get_vertex_buffer());
                m_manager.m_render.destroy_index_buffer(it->second->get_index_buffer());

                it = m_manager.m_geometries.erase(it);
            } else {
                ++it;
            }
        }
    }

    const char* get_name() const override {
        return "Geometry Manager Begin";
    }

private:
    GeometryManager& m_manager;
    Task* m_end_task;
};

GeometryManager::GeometryManager(const GeometryManagerDescriptor& descriptor)
    : m_render(*descriptor.render)
    , m_task_scheduler(*descriptor.task_scheduler)
    , m_persistent_memory_resource(*descriptor.persistent_memory_resource)
    , m_transient_memory_resource(*descriptor.transient_memory_resource)
    , m_geometries(*descriptor.persistent_memory_resource)
    , m_pending_geometries(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    m_geometries.reserve(32);
    m_pending_geometries.reserve(32);
}

GeometryManager::~GeometryManager() {
    m_pending_geometries.clear();

    for (auto& [relative_path, geometry] : m_geometries) {
        KW_ASSERT(geometry.use_count() == 1, "Not all geometries are released.");

        m_render.destroy_vertex_buffer(geometry->get_vertex_buffer());
        m_render.destroy_index_buffer(geometry->get_index_buffer());
    }
}

SharedPtr<Geometry> GeometryManager::load(const char* relative_path) {
    {
        std::shared_lock shared_lock(m_geometries_mutex);

        auto it = m_geometries.find(String(relative_path, m_transient_memory_resource));
        if (it != m_geometries.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_geometries_mutex);

        auto [it, success] = m_geometries.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<Geometry>());
        if (!success) {
            // Could return here if geometry is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<Geometry>(m_persistent_memory_resource);

        m_pending_geometries.emplace_back(it->first, it->second);

        return it->second;
    }
}

std::pair<Task*, Task*> GeometryManager::create_tasks() {
    Task* end_task = new (m_transient_memory_resource.allocate<NoopTask>()) NoopTask("Geometry Manager End");
    Task* begin_task = new (m_transient_memory_resource.allocate<BeginTask>()) BeginTask(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw