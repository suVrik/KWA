#pragma once

#include "render/particles/generators/particle_system_generator.h"

#include <core/math/float3.h>

namespace kw {

class MemoryResource;
class ObjectNode;

class CylinderPositionParticleSystemGenerator : public ParticleSystemGenerator {
public:
    static ParticleSystemGenerator* create_from_markdown(MemoryResource& memory_resource, ObjectNode& node);

    // Bottom center of the cylinder is located at particle system's origin.
    CylinderPositionParticleSystemGenerator(const float3& origin, float radius, float height);

    void generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const override;

    ParticleSystemStreamMask get_stream_mask() const override;

private:
    float3 m_origin;
    float m_radius;
    float m_height;
};

} // namespace kw
