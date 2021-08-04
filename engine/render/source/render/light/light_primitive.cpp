#include "render/light/light_primitive.h"
#include "render/container/container_primitive.h"

namespace kw {

LightPrimitive::LightPrimitive(float3 color, float power, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_bounds(float3(local_transform.translation), float3(std::sqrt(power * 10.f)))
    , m_color(color)
    , m_power(power)
{
}

LightPrimitive::~LightPrimitive() {
    // Scene must know whether the removed object is geometry, light or container to properly clean up acceleration
    // structures. If `remove_child` is called from `Primitive`, there's no way to know whether this was geometry,
    // light or container anymore.
    if (m_parent != nullptr) {
        m_parent->remove_child(*this);
    }
}

const aabbox& LightPrimitive::get_bounds() const {
    return m_bounds;
}

const float3& LightPrimitive::get_color() const {
    return m_color;
}

void LightPrimitive::set_color(const float3& value) {
    m_color = value;
}

float LightPrimitive::get_power() const {
    return m_power;
}

void LightPrimitive::set_power(float value) {
    m_bounds = aabbox(float3(m_global_transform.translation), float3(std::sqrt(value * 10.f)));

    m_power = value;
}

void LightPrimitive::global_transform_updated() {
    AccelerationStructurePrimitive::global_transform_updated();

    m_bounds = aabbox(float3(m_global_transform.translation), float3(std::sqrt(m_power * 10.f)));
}

} // namespace kw
