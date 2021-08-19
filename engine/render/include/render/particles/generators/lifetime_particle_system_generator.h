#pragma once

#include "render/particles/generators/particle_system_generator.h"

namespace kw {

class MemoryResource;
class ObjectNode;

struct LifetimeParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's lifetime is a random number in range [min_lifetime, max_lifetime].
    LifetimeParticleSystemGenerator(float min_lifetime, float max_lifetime);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float m_lifetime_range;
    float m_lifetime_offset;
};

} // namespace kw
