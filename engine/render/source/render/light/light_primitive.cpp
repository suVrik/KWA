#include "render/light/light_primitive.h"

namespace kw {

LightPrimitive::LightPrimitive(float3 color, float power, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_color(color)
    , m_power(power)
{
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
