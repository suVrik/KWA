#pragma once

#include "render/particles/updaters/over_lifetime_particle_system_updater.h"

namespace kw {

class MemoryResource;
class ObjectNode;

class AlphaOverLifetimeParticleSystemUpdater : public OverLifetimeParticleSystemUpdater<float> {
public:
    static ParticleSystemUpdater* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    AlphaOverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<float>&& outputs);

    void update(ParticleSystemPrimitive& primitive, float elapsed_time) const override;

    ParticleSystemStreamMask get_stream_mask() const override;
};

} // namespace kw
