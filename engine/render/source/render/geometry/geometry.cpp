#include "render/geometry/geometry.h"
#include "render/geometry/geometry_listener.h"
#include "render/geometry/geometry_notifier.h"
#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>

#include <atomic>

namespace kw {

Geometry::Geometry(GeometryNotifier& geometry_notifier)
    : m_geometry_notifier(geometry_notifier)
    , m_index_count(0)
    , m_index_buffer(nullptr)
    , m_skinned_vertex_buffer(nullptr)
    , m_vertex_buffer(nullptr)
{
}
    
Geometry::Geometry(GeometryNotifier& geometry_notifier, VertexBuffer* vertex_buffer, VertexBuffer* skinned_vertex_buffer,
                   IndexBuffer* index_buffer, uint32_t index_count, const aabbox& bounds, UniquePtr<Skeleton>&& skeleton)
    : m_geometry_notifier(geometry_notifier)
    , m_skeleton(std::move(skeleton))
    , m_bounds(bounds)
    , m_index_count(index_count)
    , m_index_buffer(index_buffer)
    , m_skinned_vertex_buffer(skinned_vertex_buffer)
{
    // Make other properties visible to other threads not later than `m_vertex_buffer`.
    std::atomic_thread_fence(std::memory_order_release);

    m_vertex_buffer = vertex_buffer;
}

Geometry::Geometry(Geometry&& other)
    : m_geometry_notifier(other.m_geometry_notifier)
    , m_skeleton(std::move(other.m_skeleton))
    , m_bounds(other.m_bounds)
    , m_index_count(other.m_index_count)
    , m_index_buffer(other.m_index_buffer)
    , m_skinned_vertex_buffer(other.m_skinned_vertex_buffer)
{
    // Make other properties visible to other threads not later than `m_vertex_buffer`.
    std::atomic_thread_fence(std::memory_order_release);

    m_vertex_buffer = other.m_vertex_buffer;

    other.m_vertex_buffer = nullptr;

    // Make `m_vertex_buffer` visible to other threads before any other properties. The `m_skeleton` have been changed
    // before though (via `std::move`). If that becomes a problem, consider copy constructor instead.
    std::atomic_thread_fence(std::memory_order_release);

    other.m_skinned_vertex_buffer = nullptr;
    other.m_index_buffer = nullptr;
    other.m_index_count = 0;
    other.m_bounds = {};
}

Geometry::~Geometry() = default;

Geometry& Geometry::operator=(Geometry&& other) {
    KW_ASSERT(!is_loaded(), "Move assignemt is allowed only for unloaded geometry.");

    m_skeleton = std::move(other.m_skeleton);
    m_bounds = other.m_bounds;
    m_index_count = other.m_index_count;
    m_index_buffer = other.m_index_buffer;
    m_skinned_vertex_buffer = other.m_skinned_vertex_buffer;

    // Make other properties visible to other threads not later than `m_vertex_buffer`.
    std::atomic_thread_fence(std::memory_order_release);

    m_vertex_buffer = other.m_vertex_buffer;

    other.m_vertex_buffer = nullptr;

    // Make `m_vertex_buffer` visible to other threads before any other properties. The `m_skeleton` have been changed
    // before though (via `std::move`). If that becomes a problem, consider copy constructor instead.
    std::atomic_thread_fence(std::memory_order_release);

    other.m_skinned_vertex_buffer = nullptr;
    other.m_index_buffer = nullptr;
    other.m_index_count = 0;
    other.m_bounds = {};

    return *this;
}

void Geometry::subscribe(GeometryListener& geometry_listener) {
    m_geometry_notifier.subscribe(*this, geometry_listener);
}

void Geometry::unsubscribe(GeometryListener& geometry_listener) {
    m_geometry_notifier.unsubscribe(*this, geometry_listener);
}

VertexBuffer* Geometry::get_vertex_buffer() const {
    return m_vertex_buffer;
}

VertexBuffer* Geometry::get_skinned_vertex_buffer() const {
    return m_skinned_vertex_buffer;
}

IndexBuffer* Geometry::get_index_buffer() const {
    return m_index_buffer;
}

uint32_t Geometry::get_index_count() const {
    return m_index_count;
}

const aabbox& Geometry::get_bounds() const {
    return m_bounds;
}

const Skeleton* Geometry::get_skeleton() const {
    return m_skeleton.get();
}

bool Geometry::is_loaded() const {
    return m_vertex_buffer != nullptr;
}

} // namespace kw
