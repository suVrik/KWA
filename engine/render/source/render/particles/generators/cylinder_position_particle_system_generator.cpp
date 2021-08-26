#include "render/particles/generators/cylinder_position_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

#include <emmintrin.h>
#include <immintrin.h>

namespace kw {

ParticleSystemGenerator* CylinderPositionParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<CylinderPositionParticleSystemGenerator>(
        MarkdownUtils::float3_from_markdown(node["origin"]),
        static_cast<float>(node["radius"].as<NumberNode>().get_value()),
        static_cast<float>(node["height"].as<NumberNode>().get_value())
    );
}

CylinderPositionParticleSystemGenerator::CylinderPositionParticleSystemGenerator(const float3& origin, float radius, float height)
    : m_origin(origin)
    , m_radius(radius)
    , m_height(height)
{
}

void CylinderPositionParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    const transform& global_transform = primitive.get_global_transform();
    alignas(16) float4x4 global_transform_matrix(global_transform);

    __m128 global_transform_row0_xmm = _mm_load_ps(&global_transform_matrix[0]);
    __m128 global_transform_row3_xmm = _mm_load_ps(&global_transform_matrix[3]);
    __m128 global_transform_row1_xmm = _mm_load_ps(&global_transform_matrix[1]);
    __m128 global_transform_row2_xmm = _mm_load_ps(&global_transform_matrix[2]);

    float* position_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_X);
    KW_ASSERT(position_x_stream != nullptr);

    float* position_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_Y);
    KW_ASSERT(position_y_stream != nullptr);

    float* position_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::POSITION_Z);
    KW_ASSERT(position_z_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        float height = m_height * random.rand_float();
        float radius = std::sqrt(m_radius * random.rand_float());
        float angle  = 2.f * PI * random.rand_float();

        __m128 local_point_x_xmm = _mm_set_ps1(m_origin.x + radius * std::cos(angle));
        __m128 local_point_y_xmm = _mm_set_ps1(m_origin.y + height);
        __m128 local_point_z_xmm = _mm_set_ps1(m_origin.z + radius * std::sin(angle));

        __m128 global_point_xmm = _mm_fmadd_ps(local_point_x_xmm, global_transform_row0_xmm, global_transform_row3_xmm);
        global_point_xmm = _mm_fmadd_ps(local_point_y_xmm, global_transform_row1_xmm, global_point_xmm);
        global_point_xmm = _mm_fmadd_ps(local_point_z_xmm, global_transform_row2_xmm, global_point_xmm);

        position_x_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_point_xmm, _MM_SHUFFLE(0, 0, 0, 0)));
        position_y_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_point_xmm, _MM_SHUFFLE(1, 1, 1, 1)));
        position_z_stream[i] = _mm_cvtss_f32(_mm_permute_ps(global_point_xmm, _MM_SHUFFLE(2, 2, 2, 2)));
    }
}

ParticleSystemStreamMask CylinderPositionParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::POSITION_X | ParticleSystemStreamMask::POSITION_Y | ParticleSystemStreamMask::POSITION_Z;
}

} // namespace kw
