#pragma once

#include <core/containers/unique_ptr.h>
#include <core/math/aabbox.h>
#include <core/math/float4.h>

namespace kw {

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

    Geometry();
    Geometry(VertexBuffer* vertex_buffer, VertexBuffer* skinned_vertex_buffer, IndexBuffer* index_buffer,
             uint32_t index_count, const aabbox& bounds, UniquePtr<Skeleton>&& skeleton);
    Geometry(Geometry&& other);
    ~Geometry();
    Geometry& operator=(Geometry&& other);

    VertexBuffer* get_vertex_buffer() const;
    VertexBuffer* get_skinned_vertex_buffer() const;
    IndexBuffer* get_index_buffer() const;
    uint32_t get_index_count() const;
    const aabbox& get_bounds() const;
    const Skeleton* get_skeleton() const;

private:
    VertexBuffer* m_vertex_buffer;
    VertexBuffer* m_skinned_vertex_buffer;
    IndexBuffer* m_index_buffer;
    uint32_t m_index_count;
    aabbox m_bounds;
    UniquePtr<Skeleton> m_skeleton;
};

} // namespace kw
