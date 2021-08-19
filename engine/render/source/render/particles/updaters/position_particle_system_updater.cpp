#include "render/particles/updaters/position_particle_system_updater.h"
#include "render/particles/particle_system_primitive.h"

#include <core/debug/assert.h>

#include <emmintrin.h>
#include <immintrin.h>

namespace kw {

ParticleSystemUpdater* PositionParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<PositionParticleSystemUpdater>();
}

void PositionParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    size_t particle_count = primitive.get_particle_count();

    __m128 elapsed_time_xmm = _mm_set1_ps(elapsed_time);

    float* position_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_X);
    KW_ASSERT(position_x_stream != nullptr);

    float* generated_velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_X);
    KW_ASSERT(generated_velocity_x_stream != nullptr);
    
    float* velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_X);
    KW_ASSERT(velocity_x_stream != nullptr);

    for (size_t i = 0; i < particle_count; i += 4) {
        __m128 position_xmm = _mm_load_ps(position_x_stream + i);
        __m128 generated_velocity_xmm = _mm_load_ps(generated_velocity_x_stream + i);
        __m128 velocity_xmm = _mm_load_ps(velocity_x_stream + i);
        __m128 result_xmm = _mm_fmadd_ps(_mm_mul_ps(generated_velocity_xmm, velocity_xmm), elapsed_time_xmm, position_xmm);
        _mm_store_ps(position_x_stream + i, result_xmm);
    }

    float* position_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_Y);
    KW_ASSERT(position_y_stream != nullptr);

    float* generated_velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Y);
    KW_ASSERT(generated_velocity_y_stream != nullptr);

    float* velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Y);
    KW_ASSERT(velocity_y_stream != nullptr);

    for (size_t i = 0; i < particle_count; i += 4) {
        __m128 position_xmm = _mm_load_ps(position_y_stream + i);
        __m128 generated_velocity_xmm = _mm_load_ps(generated_velocity_y_stream + i);
        __m128 velocity_xmm = _mm_load_ps(velocity_y_stream + i);
        __m128 result_xmm = _mm_fmadd_ps(_mm_mul_ps(generated_velocity_xmm, velocity_xmm), elapsed_time_xmm, position_xmm);
        _mm_store_ps(position_y_stream + i, result_xmm);
    }

    float* position_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_Z);
    KW_ASSERT(position_z_stream != nullptr);

    float* generated_velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Z);
    KW_ASSERT(generated_velocity_z_stream != nullptr);

    float* velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Z);
    KW_ASSERT(velocity_z_stream != nullptr);

    for (size_t i = 0; i < particle_count; i += 4) {
        __m128 position_xmm = _mm_load_ps(position_z_stream + i);
        __m128 generated_velocity_xmm = _mm_load_ps(generated_velocity_z_stream + i);
        __m128 velocity_xmm = _mm_load_ps(velocity_z_stream + i);
        __m128 result_xmm = _mm_fmadd_ps(_mm_mul_ps(generated_velocity_xmm, velocity_xmm), elapsed_time_xmm, position_xmm);
        _mm_store_ps(position_z_stream + i, result_xmm);
    }
}

ParticleSystemStreamMask PositionParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::POSITION_X | ParticleSystemStreamMask::POSITION_Y | ParticleSystemStreamMask::POSITION_Z |
           ParticleSystemStreamMask::VELOCITY_X | ParticleSystemStreamMask::VELOCITY_Y | ParticleSystemStreamMask::VELOCITY_Z |
           ParticleSystemStreamMask::GENERATED_VELOCITY_X | ParticleSystemStreamMask::GENERATED_VELOCITY_Y | ParticleSystemStreamMask::GENERATED_VELOCITY_Z;
}

} // namespace kw
