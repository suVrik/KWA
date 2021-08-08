#pragma once

#include <core/containers/unique_ptr.h>
#include <core/math/aabbox.h>
#include <core/math/float4.h>

namespace kw {

class GeometryListener;
class GeometryNotifier;
class IndexBuffer;
class Skeleton;
class VertexBuffer;

class Geometry {
public:
    struct Vertex {
        float3 position;
        float3 normal;
        float4 tangent;
        float2 texcoord_0;
    };

    struct SkinnedVertex {
        uint8_t joints[4];
        uint8_t weights[4];
    };

    explicit Geometry(GeometryNotifier& geometry_notifier);
    Geometry(GeometryNotifier& geometry_notifier, VertexBuffer* vertex_buffer, VertexBuffer* skinned_vertex_buffer,
             IndexBuffer* index_buffer, uint32_t index_count, const aabbox& bounds, UniquePtr<Skeleton>&& skeleton);
    Geometry(Geometry&& other);
    ~Geometry();
    Geometry& operator=(Geometry&& other);

    // This geometry listener will be notified when this geometry is loaded.
    // If this geometry is already loaded, the listener will be notified immediately.
    void subscribe(GeometryListener& geometry_listener);
    void unsubscribe(GeometryListener& geometry_listener);

    VertexBuffer* get_vertex_buffer() const;
    VertexBuffer* get_skinned_vertex_buffer() const;
    IndexBuffer* get_index_buffer() const;
    uint32_t get_index_count() const;
    const aabbox& get_bounds() const;
    const Skeleton* get_skeleton() const;

    bool is_loaded() const;

private:
    GeometryNotifier& m_geometry_notifier;

    // Geometry data is initialized in reverse order with thread fences.
    // When `m_vertex_buffer` is set, other fields are guaranteed to be set too.
    UniquePtr<Skeleton> m_skeleton;
    aabbox m_bounds;
    uint32_t m_index_count;
    IndexBuffer* m_index_buffer;
    VertexBuffer* m_skinned_vertex_buffer;
    VertexBuffer* m_vertex_buffer;
};

} // namespace kw
