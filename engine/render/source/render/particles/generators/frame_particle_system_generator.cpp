#include "render/particles/generators/frame_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemGenerator* FrameParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<FrameParticleSystemGenerator>(
        node["min"].as<NumberNode>().get_value(),
        node["max"].as<NumberNode>().get_value()
    );
}

FrameParticleSystemGenerator::FrameParticleSystemGenerator(float min_frame, float max_frame)
    : m_frame_range(max_frame - min_frame)
    , m_frame_offset(min_frame)
{
}

void FrameParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* frame_stream = primitive.get_particle_system_stream(ParticleSystemStream::FRAME);
    KW_ASSERT(frame_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        frame_stream[i] = random.rand_float() * m_frame_range + m_frame_offset;
    }
}

ParticleSystemStreamMask FrameParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::FRAME;
}

} // namespace kw
