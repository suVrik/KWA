#pragma once

#include "render/particles/emitters/particle_system_emitter.h"

namespace kw {

class MemoryResource;
class ObjectNode;

class OverLifetimeParticleSystemEmitter : public ParticleSystemEmitter {
public:
    static ParticleSystemEmitter* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // `emit_per_second_from` how many particles to emit when particle system time is 0.
    // `emit_per_second_to` how many particles to emit when particle system time is 1.
    OverLifetimeParticleSystemEmitter(float emit_per_second_from, float emit_per_second_to);

    size_t emit(const ParticleSystemPrimitive& primitive, float elapsed_time) const override;

private:
    float m_emit_per_second_half_range;
    float m_emit_per_second_offset;
};

} // namespace kw
