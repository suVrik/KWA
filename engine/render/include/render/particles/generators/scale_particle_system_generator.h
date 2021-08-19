#pragma once

#include "render/particles/generators/particle_system_generator.h"

#include <core/math/float3.h>

namespace kw {

class MemoryResource;
class ObjectNode;

struct ScaleParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Each particle's scale is a random vector in range [min_scale, max_scale].
    // If `is_uniform` is set to true, only the x component is used.
    ScaleParticleSystemGenerator(bool is_uniform, const float3& min_scale, const float3& max_scale);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    void generate_uniform(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const;
    void generate_non_uniform(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const;

    bool m_is_uniform;
    float3 m_scale_range;
    float3 m_scale_offset;
};

} // namespace kw
