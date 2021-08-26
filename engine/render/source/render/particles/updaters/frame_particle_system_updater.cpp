#include "render/particles/updaters/frame_particle_system_updater.h"
#include "render/particles/particle_system_primitive.h"

#include <core/debug/assert.h>
#include <core/io/markdown.h>

#include <emmintrin.h>

namespace kw {

ParticleSystemUpdater* FrameParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<FrameParticleSystemUpdater>(
        static_cast<float>(node["framerate"].as<NumberNode>().get_value())
    );
}

FrameParticleSystemUpdater::FrameParticleSystemUpdater(float framerate)
    : m_framerate(framerate)
{
}

void FrameParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    size_t particle_count = primitive.get_particle_count();

    float* frame_stream = primitive.get_particle_system_stream(ParticleSystemStream::FRAME);
    KW_ASSERT(frame_stream != nullptr);

    __m128 elapsed_time_xmm = _mm_set1_ps(elapsed_time * m_framerate);

    for (size_t i = 0; i < particle_count; i += 4) {
        _mm_store_ps(frame_stream + i, _mm_add_ps(_mm_load_ps(frame_stream + i), elapsed_time_xmm));
    }
}

ParticleSystemStreamMask FrameParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::FRAME;
}

} // namespace kw
