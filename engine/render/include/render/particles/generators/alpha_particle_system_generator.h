#pragma once

#include "render/particles/generators/particle_system_generator.h"

#include <core/math/float4.h>

namespace kw {

class MemoryResource;
class ObjectNode;

struct AlphaParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's alpha is a random vector in range [min_alpha, max_alpha].
    AlphaParticleSystemGenerator(float min_alpha, float max_alpha);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float m_alpha_range;
    float m_alpha_offset;
};

} // namespace kw
