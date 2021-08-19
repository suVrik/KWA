#pragma once

#include "render/particles/particle_system_stream.h"
#include "render/particles/updaters/particle_system_updater.h"

#include <core/math/float3.h>

namespace kw {

class MemoryResource;
class ObjectNode;

class ScaleBySpeedParticleSystemUpdater : public ParticleSystemUpdater {
public:
    static ParticleSystemUpdater* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    explicit ScaleBySpeedParticleSystemUpdater(const float3& speed_scale);

    void update(ParticleSystemPrimitive& primitive, float elapsed_time) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float3 m_speed_scale;
};

} // namespace kw
