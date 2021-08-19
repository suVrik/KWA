#pragma once

#include "render/particles/generators/particle_system_generator.h"

namespace kw {

class MemoryResource;
class ObjectNode;

struct FrameParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's starting frame is a random number in range [min_frame, max_frame].
    FrameParticleSystemGenerator(float min_frame, float max_frame);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float m_frame_range;
    float m_frame_offset;
};

} // namespace kw
