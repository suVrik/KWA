#include "render/light/sphere_light_primitive.h"

namespace kw {

SphereLightPrimitive::SphereLightPrimitive(float radius, bool is_shadow_enabled, float3 color, float power, const transform& local_transform)
    : LightPrimitive(color, power, local_transform)
    , m_radius(radius)
    , m_is_shadow_enabled(is_shadow_enabled)
    , m_shadow_params{ 0.005f, 0.f, 20.f, 0.6f }
{
}

float SphereLightPrimitive::get_radius() const {
    return m_radius;
}

void SphereLightPrimitive::set_radius(float value) {
    m_radius = value;
}

bool SphereLightPrimitive::is_shadow_enabled() const {
    return m_is_shadow_enabled;
}

void SphereLightPrimitive::toggle_shadow_enabled(bool value) {
    m_is_shadow_enabled = value;
}

const SphereLightPrimitive::ShadowParams& SphereLightPrimitive::get_shadow_params() const {
    return m_shadow_params;
}

void SphereLightPrimitive::set_shadow_params(const ShadowParams& value) {
    m_shadow_params = value;
}

} // namespace kw
