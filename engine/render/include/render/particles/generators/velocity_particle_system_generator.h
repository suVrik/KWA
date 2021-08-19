#pragma once

#include "render/particles/generators/particle_system_generator.h"

#include <core/math/float4.h>

namespace kw {

class MemoryResource;
class ObjectNode;

struct VelocityParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's velocity is a random vector in range [min_velocity, max_velocity].
    VelocityParticleSystemGenerator(const float3& min_velocity, const float3& max_velocity);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    alignas(16) float4 m_velocity_range;
    alignas(16) float4 m_velocity_offset;
};

} // namespace kw
