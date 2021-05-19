#pragma once

#include "render/render.h"

#include <core/math/aabbox.h>
#include <core/math/float4.h>

namespace kw {

struct KwgData {
    struct Vertex {
        float3 position;
        float3 normal;
        float4 tangent;
        float2 texcoord_0;
    };

    const Vertex* vertices;
    size_t vertex_count;

    const uint16_t* indices16;
    size_t index16_count;

    const uint32_t* indices32;
    size_t index32_count;

    const aabbox* bounds;
};

namespace RenderUtils {

TextureDescriptor load_dds(MemoryResource& memory_resource, const char* relative_path);

KwgData load_kwg(MemoryResource& memory_resource, const char* relative_path);

} // namespace RenderUtils

} // namespace kw
