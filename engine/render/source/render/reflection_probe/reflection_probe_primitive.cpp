#include "render/reflection_probe/reflection_probe_primitive.h"

namespace kw {

ReflectionProbePrimitive::ReflectionProbePrimitive(SharedPtr<Texture*> irradiance_map, SharedPtr<Texture*> prefiltered_environment_map,
                                                   float falloff_radius, const aabbox& parallax_box, const transform& local_transform)
    : AccelerationStructurePrimitive(local_transform)
    , m_reflection_probe_manager(nullptr)
    , m_irradiance_map(std::move(irradiance_map))
    , m_prefiltered_environment_map(std::move(prefiltered_environment_map))
    , m_falloff_radius(falloff_radius)
    , m_parallax_box(parallax_box)
{
    m_bounds = aabbox(get_global_translation(), float3(m_falloff_radius));
}

ReflectionProbeManager* ReflectionProbePrimitive::get_reflection_probe_manager() const {
    return m_reflection_probe_manager;
}

const SharedPtr<Texture*>& ReflectionProbePrimitive::get_irradiance_map() const {
    return m_irradiance_map;
}

void ReflectionProbePrimitive::set_irradiance_map(SharedPtr<Texture*> texture) {
    m_irradiance_map = std::move(texture);
}

const SharedPtr<Texture*>& ReflectionProbePrimitive::get_prefiltered_environment_map() const {
    return m_prefiltered_environment_map;
}

void ReflectionProbePrimitive::set_prefiltered_environment_map(SharedPtr<Texture*> texture) {
    m_prefiltered_environment_map = std::move(texture);
}

float ReflectionProbePrimitive::get_falloff_radius() const {
    return m_falloff_radius;
}

void ReflectionProbePrimitive::set_falloff_radius(float value) {
    m_falloff_radius = value;

    m_bounds = aabbox(get_global_translation(), float3(m_falloff_radius));
}

const aabbox& ReflectionProbePrimitive::get_parallax_box() const {
    return m_parallax_box;
}

void ReflectionProbePrimitive::set_parallax_box(const aabbox& value) {
    m_parallax_box = value;
}

UniquePtr<Primitive> ReflectionProbePrimitive::clone(MemoryResource& memory_resource) const {
    return static_pointer_cast<Primitive>(allocate_unique<ReflectionProbePrimitive>(memory_resource, *this));
}

void ReflectionProbePrimitive::global_transform_updated() {
    m_bounds = aabbox(get_global_translation(), float3(m_falloff_radius));

    AccelerationStructurePrimitive::global_transform_updated();
}

} // namespace kw
