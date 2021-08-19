#pragma once

#include "render/particles/particle_system_stream_mask.h"

namespace kw {

class ParticleSystemPrimitive;

class ParticleSystemUpdater {
public:
    virtual ~ParticleSystemUpdater() = default;

    // Update particular streams (as returned by `get_stream_mask`) for all particles.
    virtual void update(ParticleSystemPrimitive& primitive, float elapsed_time) const = 0;

    // This updater will update these streams. Updaters are executed in the order specified in `ParticleUpdater`.
    virtual ParticleSystemStreamMask get_stream_mask() const = 0;
};

} // namespace kw
