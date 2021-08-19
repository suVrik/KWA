#include "render/particles/updaters/scale_by_speed_particle_system_updater.h"
#include "render/particles/particle_system_primitive.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>

#include <emmintrin.h>
#include <immintrin.h>

namespace kw {

ParticleSystemUpdater* ScaleBySpeedParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    ArrayNode& speed_scale_node = node["speed_scale"].as<ArrayNode>();
    KW_ERROR(speed_scale_node.get_size() == 3, "Invalid speed_scale.");

    float3 speed_scale(static_cast<float>(speed_scale_node[0].as<NumberNode>().get_value()),
                       static_cast<float>(speed_scale_node[1].as<NumberNode>().get_value()),
                       static_cast<float>(speed_scale_node[2].as<NumberNode>().get_value()));

    return memory_resource.construct<ScaleBySpeedParticleSystemUpdater>(speed_scale);
}

ScaleBySpeedParticleSystemUpdater::ScaleBySpeedParticleSystemUpdater(const float3& speed_scale)
    : m_speed_scale(speed_scale)
{
}

void ScaleBySpeedParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    size_t particle_count = primitive.get_particle_count();

    float* generated_velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_X);
    KW_ASSERT(generated_velocity_x_stream != nullptr);

    float* generated_velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Y);
    KW_ASSERT(generated_velocity_y_stream != nullptr);

    float* generated_velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Z);
    KW_ASSERT(generated_velocity_z_stream != nullptr);

    float* velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_X);
    KW_ASSERT(velocity_x_stream != nullptr);

    float* velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Y);
    KW_ASSERT(velocity_y_stream != nullptr);

    float* velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Z);
    KW_ASSERT(velocity_z_stream != nullptr);

    float* scale_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::SCALE_X);
    KW_ASSERT(scale_x_stream != nullptr);

    float* scale_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::SCALE_Y);
    KW_ASSERT(scale_y_stream != nullptr);

    float* scale_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::SCALE_Z);
    KW_ASSERT(scale_z_stream != nullptr);

    __m128 speed_scale_x_xmm = _mm_set1_ps(m_speed_scale.x);
    __m128 speed_scale_y_xmm = _mm_set1_ps(m_speed_scale.y);
    __m128 speed_scale_z_xmm = _mm_set1_ps(m_speed_scale.z);

    for (size_t i = 0; i < particle_count; i += 4) {
        __m128 generated_velocity_x_xmm = _mm_load_ps(generated_velocity_x_stream + i);
        __m128 generated_velocity_y_xmm = _mm_load_ps(generated_velocity_y_stream + i);
        __m128 generated_velocity_z_xmm = _mm_load_ps(generated_velocity_z_stream + i);
        
        __m128 velocity_x_xmm = _mm_load_ps(velocity_x_stream + i);
        __m128 velocity_y_xmm = _mm_load_ps(velocity_y_stream + i);
        __m128 velocity_z_xmm = _mm_load_ps(velocity_z_stream + i);
        
        __m128 final_velocity_x_xmm = _mm_mul_ps(generated_velocity_x_xmm, velocity_x_xmm);
        __m128 final_velocity_y_xmm = _mm_mul_ps(generated_velocity_y_xmm, velocity_y_xmm);
        __m128 final_velocity_z_xmm = _mm_mul_ps(generated_velocity_z_xmm, velocity_z_xmm);

        __m128 speed_xmm = _mm_sqrt_ps(_mm_fmadd_ps(final_velocity_x_xmm, final_velocity_x_xmm, _mm_fmadd_ps(final_velocity_y_xmm, final_velocity_y_xmm, _mm_mul_ps(final_velocity_z_xmm, final_velocity_z_xmm))));

        _mm_store_ps(scale_x_stream + i, _mm_mul_ps(_mm_mul_ps(speed_xmm, speed_scale_x_xmm), _mm_load_ps(scale_x_stream + i)));
        _mm_store_ps(scale_y_stream + i, _mm_mul_ps(_mm_mul_ps(speed_xmm, speed_scale_y_xmm), _mm_load_ps(scale_y_stream + i)));
        _mm_store_ps(scale_z_stream + i, _mm_mul_ps(_mm_mul_ps(speed_xmm, speed_scale_z_xmm), _mm_load_ps(scale_z_stream + i)));
    }
}

ParticleSystemStreamMask ScaleBySpeedParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::SCALE_X | ParticleSystemStreamMask::SCALE_Y | ParticleSystemStreamMask::SCALE_Z |
           ParticleSystemStreamMask::VELOCITY_X | ParticleSystemStreamMask::VELOCITY_Y | ParticleSystemStreamMask::VELOCITY_Z;
}

} // namespace kw
