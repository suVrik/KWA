#include "render/particles/generators/lifetime_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemGenerator* LifetimeParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<LifetimeParticleSystemGenerator>(
        node["min"].as<NumberNode>().get_value(),
        node["max"].as<NumberNode>().get_value()
    );
}

LifetimeParticleSystemGenerator::LifetimeParticleSystemGenerator(float min_lifetime, float max_lifetime)
    : m_lifetime_range(max_lifetime - min_lifetime)
    , m_lifetime_offset(min_lifetime)
{
}

void LifetimeParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* total_lifetime_stream = primitive.get_particle_system_stream(ParticleSystemStream::TOTAL_LIFETIME);
    KW_ASSERT(total_lifetime_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        total_lifetime_stream[i] = random.rand_float() * m_lifetime_range + m_lifetime_offset;
    }

    float* current_lifetime_stream = primitive.get_particle_system_stream(ParticleSystemStream::CURRENT_LIFETIME);
    KW_ASSERT(current_lifetime_stream != nullptr);

    std::fill(current_lifetime_stream + begin_index, current_lifetime_stream + end_index, 0.f);
}

ParticleSystemStreamMask LifetimeParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::TOTAL_LIFETIME | ParticleSystemStreamMask::CURRENT_LIFETIME;
}

} // namespace kw
