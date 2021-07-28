#include "render/light/point_light_primitive.h"

namespace kw {

PointLightPrimitive::PointLightPrimitive(float radius, bool is_shadow_enabled, float3 color, float power, const transform& local_transform)
    : LightPrimitive(color, power, local_transform)
    , m_radius(radius)
    , m_is_shadow_enabled(is_shadow_enabled)
    , m_shadow_params{ 0.005f, 0.f, 20.f, 0.6f }
{
}

float PointLightPrimitive::get_radius() const {
    return m_radius;
}

void PointLightPrimitive::set_radius(float value) {
    m_radius = value;
}

bool PointLightPrimitive::is_shadow_enabled() const {
    return m_is_shadow_enabled;
}

void PointLightPrimitive::toggle_shadow_enabled(bool value) {
    m_is_shadow_enabled = value;
}

const PointLightPrimitive::ShadowParams& PointLightPrimitive::get_shadow_params() const {
    return m_shadow_params;
}

void PointLightPrimitive::set_shadow_params(const ShadowParams& value) {
    m_shadow_params = value;
}

} // namespace kw
