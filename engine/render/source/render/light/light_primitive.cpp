#include "render/light/light_primitive.h"

#include <atomic>

namespace kw {

// Declared in `render/acceleration_structure/acceleration_structure_primitive.cpp`.
extern std::atomic_uint64_t acceleration_structure_counter;

LightPrimitive::LightPrimitive(float3 color, float power, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_color(color)
    , m_power(power)
{
    m_bounds = aabbox(local_transform.translation, float3(std::sqrt(power * 10.f)));
}

const float3& LightPrimitive::get_color() const {
    return m_color;
}

void LightPrimitive::set_color(const float3& value) {
    if (m_color != value) {
        m_counter = ++acceleration_structure_counter;

        m_color = value;
    }
}

float LightPrimitive::get_power() const {
    return m_power;
}

void LightPrimitive::set_power(float value) {
    if (m_power != value) {
        m_bounds = aabbox(get_global_translation(), float3(std::sqrt(value * 50.f)));

        m_counter = ++acceleration_structure_counter;

        m_power = value;
    }
}

void LightPrimitive::global_transform_updated() {
    m_bounds = aabbox(get_global_translation(), float3(std::sqrt(m_power * 50.f)));

    AccelerationStructurePrimitive::global_transform_updated();
}

} // namespace kw
