#include "render/geometry/geometry_manager.h"
#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"
#include "render/render.h"

#include <core/concurrency/task.h>
#include <core/concurrency/task_scheduler.h>
#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/binary_reader.h>
#include <core/math/float4x4.h>
#include <core/math/transform.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

namespace EndianUtils {

static float2 swap_le(float2 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    return vector;
}

static float3 swap_le(float3 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    return vector;
}

static float4 swap_le(float4 vector) {
    vector.x = swap_le(vector.x);
    vector.y = swap_le(vector.y);
    vector.z = swap_le(vector.z);
    vector.w = swap_le(vector.w);
    return vector;
}

static float4x4 swap_le(float4x4 vector) {
    vector._r0 = swap_le(vector._r0);
    vector._r1 = swap_le(vector._r1);
    vector._r2 = swap_le(vector._r2);
    vector._r3 = swap_le(vector._r3);
    return vector;
}

static quaternion swap_le(quaternion value) {
    value.x = swap_le(value.x);
    value.y = swap_le(value.y);
    value.z = swap_le(value.z);
    value.w = swap_le(value.w);
    return value;
}

static transform swap_le(transform value) {
    value.translation = swap_le(value.translation);
    value.rotation = swap_le(value.rotation);
    value.scale = swap_le(value.scale);
    return value;
}

static Geometry::Vertex swap_le(Geometry::Vertex vertex) {
    vertex.position = swap_le(vertex.position);
    vertex.normal = swap_le(vertex.normal);
    vertex.tangent = swap_le(vertex.tangent);
    vertex.texcoord_0 = swap_le(vertex.texcoord_0);
    return vertex;
}

} // namespace EndianUtils

constexpr uint32_t KWG_SIGNATURE = ' GWK';

class GeometryManager::WorkerTask final : public Task {
public:
    WorkerTask(GeometryManager& manager, Geometry& geometry, const char* relative_path)
        : m_manager(manager)
        , m_geometry(geometry)
        , m_relative_path(relative_path)
    {
    }

    void run() override {
        BinaryReader reader(m_relative_path);
        KW_ERROR(reader, "Failed to open geometry \"%s\".", m_relative_path);
        KW_ERROR(read_next(reader) == KWG_SIGNATURE, "Invalid geometry \"%s\" signature.", m_relative_path);

        uint32_t vertex_count = read_next(reader);
        uint32_t skinned_vertex_count = read_next(reader);
        uint32_t index_count = read_next(reader);
        uint32_t joint_count = read_next(reader);

        aabbox bounds;
        KW_ERROR(reader.read_le<float>(bounds.data, std::size(bounds.data)), "Failed to read geometry header.");

        Vector<Geometry::Vertex> vertices(vertex_count, m_manager.m_transient_memory_resource);
        KW_ERROR(reader.read_le<Geometry::Vertex>(vertices.data(), vertices.size()), "Failed to read geometry vertices.");

        VertexBuffer* vertex_buffer = m_manager.m_render.create_vertex_buffer(m_relative_path, sizeof(Geometry::Vertex) * vertices.size());
        KW_ASSERT(vertex_buffer != nullptr);

        m_manager.m_render.upload_vertex_buffer(vertex_buffer, vertices.data(), sizeof(Geometry::Vertex) * vertices.size());

        VertexBuffer* skinned_vertex_buffer = nullptr;

        if (skinned_vertex_count > 0) {
            KW_ERROR(vertex_count == skinned_vertex_count, "Mismatching geometry vertex count.");

            Vector<Geometry::SkinnedVertex> skinned_vertices(skinned_vertex_count, m_manager.m_transient_memory_resource);
            KW_ERROR(reader.read(skinned_vertices.data(), skinned_vertices.size() * sizeof(Geometry::SkinnedVertex)), "Failed to read geometry skinned vertices.");

            skinned_vertex_buffer = m_manager.m_render.create_vertex_buffer(m_relative_path, sizeof(Geometry::SkinnedVertex) * skinned_vertices.size());
            KW_ASSERT(skinned_vertex_buffer != nullptr);

            m_manager.m_render.upload_vertex_buffer(skinned_vertex_buffer, skinned_vertices.data(), sizeof(Geometry::SkinnedVertex) * skinned_vertices.size());
        }

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

        UniquePtr<Skeleton> skeleton;

        if (joint_count > 0) {
            Vector<uint32_t> parent_joints(joint_count, m_manager.m_persistent_memory_resource);
            KW_ERROR(reader.read_le<uint32_t>(parent_joints.data(), parent_joints.size()), "Failed to read parent joint indices.");

            Vector<float4x4> inverse_bind_matrices(joint_count, m_manager.m_persistent_memory_resource);
            KW_ERROR(reader.read_le<float4x4>(inverse_bind_matrices.data(), inverse_bind_matrices.size()), "Failed to read inverse bind matrices.");

            Vector<transform> bind_transforms(joint_count, m_manager.m_persistent_memory_resource);
            KW_ERROR(reader.read_le<transform>(bind_transforms.data(), bind_transforms.size()), "Failed to read bind transforms.");

            UnorderedMap<String, uint32_t> joint_mapping(m_manager.m_persistent_memory_resource);
            joint_mapping.reserve(joint_count);

            for (uint32_t i = 0; i < joint_count; i++) {
                std::optional<uint32_t> name_length = reader.read_le<uint32_t>();
                KW_ERROR(name_length, "Failed to read joint name length.");

                String name(*name_length, '\0', m_manager.m_persistent_memory_resource);
                KW_ERROR(reader.read(name.data(), name.size()), "Failed to read joint name.");

                joint_mapping.emplace(std::move(name), i);
            }

            skeleton = allocate_unique<Skeleton>(
                m_manager.m_persistent_memory_resource,
                std::move(parent_joints), std::move(inverse_bind_matrices),
                std::move(bind_transforms), std::move(joint_mapping)
            );
        }

        m_geometry = Geometry(m_manager.m_geometry_notifier, vertex_buffer, skinned_vertex_buffer, index_buffer, index_count, bounds, std::move(skeleton));

        m_manager.m_geometry_notifier.notify(m_geometry);
    }

    const char* get_name() const override {
        return "Geometry Manager Worker";
    }

private:
    uint32_t read_next(BinaryReader& reader) {
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
        // Tasks that load geometry are expected to run before begin task, so this shouldn't block anyone.
        std::lock_guard lock_guard(m_manager.m_geometry_mutex);

        //
        // Destroy geometry that only referenced from `GeometryManager`.
        //

        for (auto it = m_manager.m_geometry.begin(); it != m_manager.m_geometry.end(); ) {
            if (it->second.use_count() == 1) {
                m_manager.m_render.destroy_vertex_buffer(it->second->get_skinned_vertex_buffer());
                m_manager.m_render.destroy_vertex_buffer(it->second->get_vertex_buffer());
                m_manager.m_render.destroy_index_buffer(it->second->get_index_buffer());

                it = m_manager.m_geometry.erase(it);
            } else {
                ++it;
            }
        }

        //
        // Start loading brand new geometry.
        //

        for (auto& [relative_path, geometry] : m_manager.m_pending_geometry) {
            WorkerTask* pending_task = m_manager.m_transient_memory_resource.construct<WorkerTask>(m_manager, *geometry, relative_path.c_str());
            KW_ASSERT(pending_task != nullptr);

            pending_task->add_output_dependencies(m_manager.m_transient_memory_resource, { m_end_task });

            m_manager.m_task_scheduler.enqueue_task(m_manager.m_transient_memory_resource, pending_task);
        }

        m_manager.m_pending_geometry.clear();
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
    , m_geometry(*descriptor.persistent_memory_resource)
    , m_pending_geometry(*descriptor.persistent_memory_resource)
    , m_geometry_notifier(*descriptor.persistent_memory_resource)
{
    KW_ASSERT(descriptor.render != nullptr);
    KW_ASSERT(descriptor.task_scheduler != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);
    KW_ASSERT(descriptor.transient_memory_resource != nullptr);

    m_geometry.reserve(32);
    m_pending_geometry.reserve(32);
}

GeometryManager::~GeometryManager() {
    m_pending_geometry.clear();

    for (auto& [relative_path, geometry] : m_geometry) {
        KW_ASSERT(geometry.use_count() == 1, "Not all geometry are released.");

        m_render.destroy_vertex_buffer(geometry->get_skinned_vertex_buffer());
        m_render.destroy_vertex_buffer(geometry->get_vertex_buffer());
        m_render.destroy_index_buffer(geometry->get_index_buffer());
    }
}

SharedPtr<Geometry> GeometryManager::load(const char* relative_path) {
    if (relative_path[0] == '\0') {
        // Empty string is allowed.
        return nullptr;
    }

    {
        std::shared_lock shared_lock(m_geometry_mutex);

        auto it = m_geometry.find(String(relative_path, m_transient_memory_resource));
        if (it != m_geometry.end()) {
            return it->second;
        }
    }

    {
        std::lock_guard lock_guard(m_geometry_mutex);

        auto [it, success] = m_geometry.emplace(String(relative_path, m_persistent_memory_resource), SharedPtr<Geometry>());
        if (!success) {
            // Could return here if geometry is enqueued from multiple threads.
            return it->second;
        }

        it->second = allocate_shared<Geometry>(m_persistent_memory_resource, m_geometry_notifier);

        m_pending_geometry.emplace_back(it->first, it->second);

        return it->second;
    }
}

const String& GeometryManager::get_relative_path(const SharedPtr<Geometry>& geometry) const {
    std::shared_lock shared_lock(m_geometry_mutex);

    for (auto& [relative_path, stored_geometry] : m_geometry) {
        if (geometry == stored_geometry) {
            return relative_path;
        }
    }

    static String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

Pair<Task*, Task*> GeometryManager::create_tasks() {
    Task* end_task = m_transient_memory_resource.construct<NoopTask>("Geometry Manager End");
    Task* begin_task = m_transient_memory_resource.construct<BeginTask>(*this, end_task);

    return { begin_task, end_task };
}

} // namespace kw
