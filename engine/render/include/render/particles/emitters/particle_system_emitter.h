#pragma once

namespace kw {

class ParticleSystemPrimitive;

class ParticleSystemEmitter {
public:
    virtual ~ParticleSystemEmitter() = default;

    // Returns the number of particles that must have been emitted since the last frame.
    virtual size_t emit(const ParticleSystemPrimitive& primitive, float elapsed_time) const = 0;
};

} // namespace kw
