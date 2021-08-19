#pragma once

#include "render/particles/updaters/over_lifetime_particle_system_updater.h"

#include <core/math/float3.h>

namespace kw {

class MemoryResource;
class ObjectNode;

class ScaleOverLifetimeParticleSystemUpdater : public OverLifetimeParticleSystemUpdater<float3> {
public:
    static ParticleSystemUpdater* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    ScaleOverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<float3>&& outputs);

    void update(ParticleSystemPrimitive& primitive, float elapsed_time) const override;

    ParticleSystemStreamMask get_stream_mask() const override;
};

} // namespace kw
