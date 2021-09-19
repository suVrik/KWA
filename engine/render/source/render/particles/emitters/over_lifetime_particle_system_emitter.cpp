#include "render/particles/emitters/over_lifetime_particle_system_emitter.h"
#include "render/particles/particle_system.h"
#include "render/particles/particle_system_primitive.h"

#include <core/io/markdown.h>

namespace kw {

ParticleSystemEmitter* OverLifetimeParticleSystemEmitter::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<OverLifetimeParticleSystemEmitter>(
        node["from"].as<NumberNode>().get_value(),
        node["to"].as<NumberNode>().get_value()
    );
}

OverLifetimeParticleSystemEmitter::OverLifetimeParticleSystemEmitter(float emit_per_second_from, float emit_per_second_to)
    : m_emit_per_second_half_range((emit_per_second_to - emit_per_second_from) / 2.f)
    , m_emit_per_second_offset(emit_per_second_from)
{
}

size_t OverLifetimeParticleSystemEmitter::emit(const ParticleSystemPrimitive& primitive, float elapsed_time) const {
    const SharedPtr<ParticleSystem>& particle_system = primitive.get_particle_system();

    float particle_system_time = primitive.get_particle_system_time();
    float data_time = particle_system->get_duration();

    float current_time  = clamp(particle_system_time, 0.f, data_time);
    float previous_time = clamp(particle_system_time - elapsed_time, 0.f, data_time);

    float current_factor  = current_time / data_time;
    float previous_factor = previous_time / data_time;

    float current_emitted  = (current_factor * m_emit_per_second_half_range + m_emit_per_second_offset) * current_time;
    float previous_emitted = (previous_factor * m_emit_per_second_half_range + m_emit_per_second_offset) * previous_time;

    return static_cast<size_t>(current_emitted) - static_cast<size_t>(previous_emitted);
}

} // namespace kw
