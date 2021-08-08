#pragma once

#include <core/math/aabbox.h>
#include <core/math/float4x4.h>

#include <atomic>

namespace kw {

class MemoryResource;

class DebugDrawManager {
public:
    explicit DebugDrawManager(MemoryResource& transient_memory_resource);

    // Must be called every frame before any debug draw primitive call.
    void update();

    // All these are lock-free.
    void line(const float3& from, const float3& to, const float3& color);
    void abbox(const aabbox& bounds, const float3& color);
    void icosahedron(const float3& center, float radius, const float3& color);
    void frustum(const float4x4& inverse_transform, const float3& color);

private:
    struct Line {
        float3 from;
        float3 to;
        float3 color;

        Line* previous;
    };

    MemoryResource& m_transient_memory_resource;
    std::atomic<Line*> m_last_line;

    friend class DebugDrawRenderPass;
};

} // namespace kw
