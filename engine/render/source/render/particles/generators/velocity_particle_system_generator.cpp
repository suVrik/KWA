#include "render/particles/generators/velocity_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>

#include <emmintrin.h>
#include <immintrin.h>

namespace kw {

ParticleSystemGenerator* VelocityParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    ArrayNode& min_node = node["min"].as<ArrayNode>();
    KW_ERROR(min_node.get_size() == 3, "Invalid min.");

    float3 min(static_cast<float>(min_node[0].as<NumberNode>().get_value()),
               static_cast<float>(min_node[1].as<NumberNode>().get_value()),
               static_cast<float>(min_node[2].as<NumberNode>().get_value()));

    ArrayNode& max_node = node["max"].as<ArrayNode>();
    KW_ERROR(max_node.get_size() == 3, "Invalid max.");

    float3 max(static_cast<float>(max_node[0].as<NumberNode>().get_value()),
               static_cast<float>(max_node[1].as<NumberNode>().get_value()),
               static_cast<float>(max_node[2].as<NumberNode>().get_value()));

    return memory_resource.construct<VelocityParticleSystemGenerator>(min, max);
}

VelocityParticleSystemGenerator::VelocityParticleSystemGenerator(const float3& min_velocity, const float3& max_velocity)
    : m_velocity_range(max_velocity - min_velocity, 0.f)
    , m_velocity_offset(min_velocity, 0.f)
{
}

void VelocityParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    __m128 velocity_range_xmm = _mm_load_ps(&m_velocity_range);
    __m128 velocity_offset_xmm = _mm_load_ps(&m_velocity_offset);

    const transform& global_transform = primitive.get_global_transform();
    alignas(16) float4x4 global_transform_matrix(global_transform);

    __m128 global_transform_row0_xmm = _mm_load_ps(&global_transform_matrix[0]);
    __m128 global_transform_row1_xmm = _mm_load_ps(&global_transform_matrix[1]);
    __m128 global_transform_row2_xmm = _mm_load_ps(&global_transform_matrix[2]);

    float* generated_velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_X);
    KW_ASSERT(generated_velocity_x_stream != nullptr);

    float* generated_velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Y);
    KW_ASSERT(generated_velocity_y_stream != nullptr);

    float* generated_velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_VELOCITY_Z);
    KW_ASSERT(generated_velocity_z_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        __m128 local_direction_xmm = _mm_fmadd_ps(random.rand_simd4(), velocity_range_xmm, velocity_offset_xmm);
        __m128 local_direction_x_xmm = _mm_permute_ps(local_direction_xmm, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 global_direction_xmm = _mm_mul_ps(local_direction_x_xmm, global_transform_row0_xmm);
        __m128 local_direction_y_xmm = _mm_permute_ps(local_direction_xmm, _MM_SHUFFLE(1, 1, 1, 1));
        global_direction_xmm = _mm_fmadd_ps(local_direction_y_xmm, global_transform_row1_xmm, global_direction_xmm);
        __m128 local_direction_z_xmm = _mm_permute_ps(local_direction_xmm, _MM_SHUFFLE(2, 2, 2, 2));
        global_direction_xmm = _mm_fmadd_ps(local_direction_z_xmm, global_transform_row2_xmm, global_direction_xmm);

        generated_velocity_x_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_direction_xmm, _MM_SHUFFLE(0, 0, 0, 0)));
        generated_velocity_y_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_direction_xmm, _MM_SHUFFLE(1, 1, 1, 1)));
        generated_velocity_z_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_direction_xmm, _MM_SHUFFLE(2, 2, 2, 2)));
    }

    float* velocity_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_X);
    KW_ASSERT(velocity_x_stream != nullptr);

    std::fill(velocity_x_stream + begin_index, velocity_x_stream + end_index, 1.f);

    float* velocity_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Y);
    KW_ASSERT(velocity_y_stream != nullptr);

    std::fill(velocity_y_stream + begin_index, velocity_y_stream + end_index, 1.f);

    float* velocity_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::VELOCITY_Z);
    KW_ASSERT(velocity_z_stream != nullptr);

    std::fill(velocity_z_stream + begin_index, velocity_z_stream + end_index, 1.f);
}

ParticleSystemStreamMask VelocityParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::VELOCITY_X | ParticleSystemStreamMask::VELOCITY_Y | ParticleSystemStreamMask::VELOCITY_Z;
}

} // namespace kw
