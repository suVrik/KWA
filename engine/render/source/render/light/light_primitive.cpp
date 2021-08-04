#include "render/light/light_primitive.h"
#include "render/container/container_primitive.h"

namespace kw {

LightPrimitive::LightPrimitive(float3 color, float power, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
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
    m_power = value;
}

} // namespace kw
