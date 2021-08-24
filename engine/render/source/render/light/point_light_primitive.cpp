#include "render/light/point_light_primitive.h"

#include <atomic>

namespace kw {

// Declared in `render/acceleration_structure/acceleration_structure_primitive.cpp`.
extern std::atomic_uint64_t acceleration_structure_counter;

PointLightPrimitive::PointLightPrimitive(bool is_shadow_enabled, float3 color, float power, const transform& local_transform)
    : LightPrimitive(color, power, local_transform)
    , m_is_shadow_enabled(is_shadow_enabled)
    , m_shadow_params{ 0.005f, 0.f, 6.f, 0.6f }
{
}

bool PointLightPrimitive::is_shadow_enabled() const {
    return m_is_shadow_enabled;
}

void PointLightPrimitive::toggle_shadow_enabled(bool value) {
    if (m_is_shadow_enabled != value) {
        m_counter = ++acceleration_structure_counter;

        m_is_shadow_enabled = value;
    }
}

const PointLightPrimitive::ShadowParams& PointLightPrimitive::get_shadow_params() const {
    return m_shadow_params;
}

void PointLightPrimitive::set_shadow_params(const ShadowParams& value) {
    if (m_shadow_params.normal_bias != value.normal_bias ||
        m_shadow_params.perspective_bias != value.perspective_bias ||
        m_shadow_params.pcss_radius != value.pcss_radius ||
        m_shadow_params.pcss_filter_factor != value.pcss_filter_factor)
    {
        m_counter = ++acceleration_structure_counter;

        m_shadow_params = value;
    }
}

} // namespace kw
