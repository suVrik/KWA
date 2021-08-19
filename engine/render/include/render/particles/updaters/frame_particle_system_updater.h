#pragma once

#include "render/particles/updaters/particle_system_updater.h"

namespace kw {

class MemoryResource;
class ObjectNode;

class FrameParticleSystemUpdater : public ParticleSystemUpdater {
public:
    static ParticleSystemUpdater* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    explicit FrameParticleSystemUpdater(float framerate);

    void update(ParticleSystemPrimitive& primitive, float elapsed_time) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float m_framerate;
};

} // namespace kw
