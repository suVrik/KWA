#pragma once

#include "render/particles/generators/particle_system_generator.h"

#include <core/math/float3.h>

namespace kw {

class MemoryResource;
class ObjectNode;

struct ColorParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's color is a random vector in range [min_color, max_color].
    ColorParticleSystemGenerator(const float3& min_color, const float3& max_color);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float3 m_color_range;
    float3 m_color_offset;
};

} // namespace kw
