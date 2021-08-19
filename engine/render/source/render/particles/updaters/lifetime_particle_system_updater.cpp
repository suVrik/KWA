#include "render/particles/updaters/lifetime_particle_system_updater.h"
#include "render/particles/particle_system_primitive.h"

#include <core/debug/assert.h>

#include <emmintrin.h>

namespace kw {

ParticleSystemUpdater* LifetimeParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<LifetimeParticleSystemUpdater>();
}

void LifetimeParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    size_t particle_count = primitive.get_particle_count();

    float* current_lifetime_stream = primitive.get_particle_system_stream(ParticleSystemStream::CURRENT_LIFETIME);
    KW_ASSERT(current_lifetime_stream != nullptr);

    __m128 elapsed_time_xmm = _mm_set1_ps(elapsed_time);

    for (size_t i = 0; i < particle_count; i += 4) {
        _mm_store_ps(current_lifetime_stream + i, _mm_add_ps(_mm_load_ps(current_lifetime_stream + i), elapsed_time_xmm));
    }
}

ParticleSystemStreamMask LifetimeParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::CURRENT_LIFETIME;
}

} // namespace kw
