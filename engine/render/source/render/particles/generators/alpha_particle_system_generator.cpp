#include "render/particles/generators/alpha_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemGenerator* AlphaParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    float min = static_cast<float>(node["min"].as<NumberNode>().get_value());
    float max = static_cast<float>(node["max"].as<NumberNode>().get_value());
    return memory_resource.construct<AlphaParticleSystemGenerator>(min, max);
}

AlphaParticleSystemGenerator::AlphaParticleSystemGenerator(float min_alpha, float max_alpha)
    : m_alpha_range(max_alpha - min_alpha)
    , m_alpha_offset(min_alpha)
{
}

void AlphaParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* color_a_stream = primitive.get_particle_system_stream(ParticleSystemStream::COLOR_A);
    KW_ASSERT(color_a_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        color_a_stream[i] = random.rand_float() * m_alpha_range + m_alpha_offset;
    }
}

ParticleSystemStreamMask AlphaParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::COLOR_A;
}

} // namespace kw
