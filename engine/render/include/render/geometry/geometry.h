#pragma once

#include <core/math/aabbox.h>

namespace kw {

class IndexBuffer;
class VertexBuffer;

class Geometry {
public:
    struct Vertex {
        float3 position;
        float3 normal;
        float4 tangent;
        float2 texcoord_0;
    };

    struct Joint {
        uint8_t joints[4];
        uint8_t weights[4];
    };

    Geometry()
        : m_vertex_buffer(nullptr)
        , m_index_buffer(nullptr)
        , m_index_count(0)
        , m_bounds{}
    {
    }
    
    Geometry(VertexBuffer* vertex_buffer, IndexBuffer* index_buffer, uint32_t index_count, const aabbox& bounds)
        : m_vertex_buffer(vertex_buffer)
        , m_index_buffer(index_buffer)
        , m_index_count(index_count)
        , m_bounds(bounds)
    {
    }

    VertexBuffer* get_vertex_buffer() const {
        return m_vertex_buffer;
    }

    IndexBuffer* get_index_buffer() const {
        return m_index_buffer;
    }

    uint32_t get_index_count() const {
        return m_index_count;
    }

    const aabbox& get_bounds() const {
        return m_bounds;
    }

private:
    VertexBuffer* m_vertex_buffer;
    IndexBuffer* m_index_buffer;
    uint32_t m_index_count;
    aabbox m_bounds;
};

} // namespace kw
