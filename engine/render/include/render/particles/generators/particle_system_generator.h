#pragma once

#include "render/particles/particle_system_stream_mask.h"

namespace kw {

class ParticleSystemPrimitive;

class ParticleSystemGenerator {
public:
    virtual ~ParticleSystemGenerator() = default;

    // Generate data for particular streams (as returned by `get_stream_mask`) for a given range of particles.
    virtual void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const = 0;

    // This generator will initialize these streams. Only one generator per stream is allowed.
    virtual ParticleSystemStreamMask get_stream_mask() const = 0;
};

} // namespace kw
