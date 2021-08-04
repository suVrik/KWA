#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>

namespace kw {

Geometry::Geometry()
    : m_vertex_buffer(nullptr)
    , m_skinned_vertex_buffer(nullptr)
    , m_index_buffer(nullptr)
    , m_index_count(0)
    , m_bounds{}
{
}
    
Geometry::Geometry(VertexBuffer* vertex_buffer, VertexBuffer* skinned_vertex_buffer, IndexBuffer* index_buffer,
                   uint32_t index_count, const aabbox& bounds, UniquePtr<Skeleton>&& skeleton)
    : m_vertex_buffer(vertex_buffer)
    , m_skinned_vertex_buffer(skinned_vertex_buffer)
    , m_index_buffer(index_buffer)
    , m_index_count(index_count)
    , m_bounds(bounds)
    , m_skeleton(std::move(skeleton))
{
}

Geometry::Geometry(Geometry&& other)
    : m_vertex_buffer(other.m_vertex_buffer)
    , m_skinned_vertex_buffer(other.m_skinned_vertex_buffer)
    , m_index_buffer(other.m_index_buffer)
    , m_index_count(other.m_index_count)
    , m_bounds(other.m_bounds)
    , m_skeleton(std::move(other.m_skeleton))
{
    other.m_vertex_buffer = nullptr;
    other.m_skinned_vertex_buffer = nullptr;
    other.m_index_buffer = nullptr;
    other.m_index_count = 0;
    other.m_bounds = {};
}

Geometry::~Geometry() = default;

Geometry& Geometry::operator=(Geometry&& other) {
    KW_ASSERT(m_vertex_buffer == nullptr, "Move assignemt is allowed only for invalid geometry.");
    KW_ASSERT(m_skinned_vertex_buffer == nullptr, "Move assignemt is allowed only for invalid geometry.");
    KW_ASSERT(m_index_buffer == nullptr, "Move assignemt is allowed only for invalid geometry.");

    m_vertex_buffer = other.m_vertex_buffer;
    m_skinned_vertex_buffer = other.m_skinned_vertex_buffer;
    m_index_buffer = other.m_index_buffer;
    m_index_count = other.m_index_count;
    m_bounds = other.m_bounds;
    m_skeleton = std::move(other.m_skeleton);

    other.m_vertex_buffer = nullptr;
    other.m_skinned_vertex_buffer = nullptr;
    other.m_index_buffer = nullptr;
    other.m_index_count = 0;
    other.m_bounds = {};

    return *this;
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

} // namespace kw
