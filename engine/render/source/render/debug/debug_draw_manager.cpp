#include "render/debug/debug_draw_manager.h"

#include <core/memory/memory_resource.h>

#include <array>
#include <utility>

namespace kw {

static const float3 ICOSAHEDRON_VERTICES[] = {
    float3( 0.00000000f,  1.00000000f,  0.00000000f), float3( 0.89442719f,  0.44721359f,  0.00000000f),
    float3( 0.27639320f,  0.44721359f,  0.85065080f), float3(-0.72360679f,  0.44721359f,  0.52573111f),
    float3(-0.72360679f,  0.44721359f, -0.52573111f), float3( 0.27639320f,  0.44721359f, -0.85065080f),
    float3( 0.00000000f, -1.00000000f,  0.00000000f), float3(-0.89442719f, -0.44721359f,  0.00000000f),
    float3(-0.27639320f, -0.44721359f, -0.85065080f), float3( 0.72360679f, -0.44721359f, -0.52573111f),
    float3( 0.72360679f, -0.44721359f,  0.52573111f), float3(-0.27639320f, -0.44721359f,  0.85065080f),
};

static const std::pair<size_t, size_t> ICOSAHEDRON_EDGES[] = {
    { 0,  1  }, { 0,  2  }, { 0,  3  }, { 0,  4  }, { 0,  5  }, { 1,  2  },
    { 1,  5  }, { 1,  9  }, { 1,  10 }, { 2,  3  }, { 2,  10 }, { 2,  11 },
    { 3,  4  }, { 3,  7  }, { 3,  11 }, { 4,  5  }, { 4,  7  }, { 4,  8  },
    { 5,  8  }, { 5,  9  }, { 6,  7  }, { 6,  8  }, { 6,  9  }, { 6,  10 },
    { 6,  11 }, { 7,  8  }, { 7,  11 }, { 8,  9  }, { 9,  10 }, { 10, 11 },
};

static const float3 FRUSTUM_VERTICES[] = {
    float3(-1.f,  1.f, 0.f), float3( 1.f,  1.f, 0.f), float3(-1.f, -1.f, 0.f), float3( 1.f, -1.f, 0.f),
    float3(-1.f,  1.f, 1.f), float3( 1.f,  1.f, 1.f), float3(-1.f, -1.f, 1.f), float3( 1.f, -1.f, 1.f),
};

static const std::pair<size_t, size_t> FRUSTUM_EDGES[] = {
    { 0, 1 }, { 0, 2 }, { 0, 4 }, { 1, 3 }, { 1, 5 }, { 2, 3 },
    { 2, 6 }, { 3, 7 }, { 4, 5 }, { 4, 6 }, { 5, 7 }, { 6, 7 },
};

DebugDrawManager::DebugDrawManager(MemoryResource& transient_memory_resource)
    : m_transient_memory_resource(transient_memory_resource)
{
}

void DebugDrawManager::update() {
    m_last_line = nullptr;
}

void DebugDrawManager::line(const float3& from, const float3& to, const float3& color) {
    Line* line = m_transient_memory_resource.allocate<Line>();
    line->from = from;
    line->to = to;
    line->color = color;
    line->previous = m_last_line.load(std::memory_order_relaxed);

    while (!m_last_line.compare_exchange_weak(line->previous, line, std::memory_order_release, std::memory_order_relaxed));
}

void DebugDrawManager::abbox(const aabbox& bounds, const float3& color) {
    line(bounds.center + float3(-bounds.extent.x, -bounds.extent.y,  bounds.extent.z), bounds.center + float3(-bounds.extent.x, -bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3(-bounds.extent.x,  bounds.extent.y, -bounds.extent.z), bounds.center + float3(-bounds.extent.x, -bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3(-bounds.extent.x,  bounds.extent.y,  bounds.extent.z), bounds.center + float3(-bounds.extent.x, -bounds.extent.y,  bounds.extent.z), color);
    line(bounds.center + float3(-bounds.extent.x,  bounds.extent.y,  bounds.extent.z), bounds.center + float3(-bounds.extent.x,  bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x, -bounds.extent.y, -bounds.extent.z), bounds.center + float3(-bounds.extent.x, -bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x, -bounds.extent.y,  bounds.extent.z), bounds.center + float3(-bounds.extent.x, -bounds.extent.y,  bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x, -bounds.extent.y,  bounds.extent.z), bounds.center + float3( bounds.extent.x, -bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x,  bounds.extent.y, -bounds.extent.z), bounds.center + float3(-bounds.extent.x,  bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x,  bounds.extent.y, -bounds.extent.z), bounds.center + float3( bounds.extent.x, -bounds.extent.y, -bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x,  bounds.extent.y,  bounds.extent.z), bounds.center + float3(-bounds.extent.x,  bounds.extent.y,  bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x,  bounds.extent.y,  bounds.extent.z), bounds.center + float3( bounds.extent.x, -bounds.extent.y,  bounds.extent.z), color);
    line(bounds.center + float3( bounds.extent.x,  bounds.extent.y,  bounds.extent.z), bounds.center + float3( bounds.extent.x,  bounds.extent.y, -bounds.extent.z), color);
}

void DebugDrawManager::icosahedron(const float3& center, float radius, const float3& color) {
    Line* lines = m_transient_memory_resource.allocate<Line>(std::size(ICOSAHEDRON_EDGES));

    for (size_t i = 0; i < std::size(ICOSAHEDRON_EDGES); i++) {
        const auto& [from, to] = ICOSAHEDRON_EDGES[i];

        lines[i].from = center + ICOSAHEDRON_VERTICES[from] * radius;
        lines[i].to = center + ICOSAHEDRON_VERTICES[to] * radius;
        lines[i].color = color;
        lines[i].previous = i > 0 ? &lines[i - 1] : m_last_line.load(std::memory_order_relaxed);
    }

    while (!m_last_line.compare_exchange_weak(lines[0].previous, &lines[std::size(ICOSAHEDRON_EDGES) - 1], std::memory_order_release, std::memory_order_relaxed));
}

void DebugDrawManager::frustum(const float4x4& inverse_transform, const float3& color) {
    Line* lines = m_transient_memory_resource.allocate<Line>(std::size(FRUSTUM_EDGES));

    for (size_t i = 0; i < std::size(FRUSTUM_EDGES); i++) {
        const auto& [from, to] = FRUSTUM_EDGES[i];

        lines[i].from = point_transform(FRUSTUM_VERTICES[from], inverse_transform);
        lines[i].to = point_transform(FRUSTUM_VERTICES[to], inverse_transform);
        lines[i].color = color;
        lines[i].previous = i > 0 ? &lines[i - 1] : m_last_line.load(std::memory_order_relaxed);
    }

    while (!m_last_line.compare_exchange_weak(lines[0].previous, &lines[std::size(FRUSTUM_EDGES) - 1], std::memory_order_release, std::memory_order_relaxed));
}

} // namespace kw
