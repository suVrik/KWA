#include "render/particles/generators/scale_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

namespace kw {

ParticleSystemGenerator* ScaleParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    return memory_resource.construct<ScaleParticleSystemGenerator>(
        node["is_uniform"].as<BooleanNode>().get_value(),
        MarkdownUtils::float3_from_markdown(node["min"]),
        MarkdownUtils::float3_from_markdown(node["max"])
    );
}

ScaleParticleSystemGenerator::ScaleParticleSystemGenerator(bool is_uniform, const float3& min_scale, const float3& max_scale)
    : m_scale_range(max_scale - min_scale)
    , m_scale_offset(min_scale)
    , m_is_uniform(is_uniform)
{
}

void ScaleParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    if (m_is_uniform) {
        generate_uniform(primitive, begin_index, end_index);
    } else {
        generate_non_uniform(primitive, begin_index, end_index);
    }
}

ParticleSystemStreamMask ScaleParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::GENERATED_SCALE_X | ParticleSystemStreamMask::GENERATED_SCALE_Y | ParticleSystemStreamMask::GENERATED_SCALE_Z;
}

void ScaleParticleSystemGenerator::generate_uniform(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* scale_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_X);
    KW_ASSERT(scale_x_stream != nullptr);

    float* scale_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Y);
    KW_ASSERT(scale_y_stream != nullptr);

    float* scale_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Z);
    KW_ASSERT(scale_z_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        float scale = random.rand_float();
        scale_x_stream[i] = scale * m_scale_range.r + m_scale_offset.r;
        scale_y_stream[i] = scale * m_scale_range.g + m_scale_offset.g;
        scale_z_stream[i] = scale * m_scale_range.b + m_scale_offset.b;
    }
}

void ScaleParticleSystemGenerator::generate_non_uniform(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* scale_x_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_X);
    KW_ASSERT(scale_x_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        scale_x_stream[i] = random.rand_float() * m_scale_range.r + m_scale_offset.r;
    }

    float* scale_y_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Y);
    KW_ASSERT(scale_y_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        scale_y_stream[i] = random.rand_float() * m_scale_range.g + m_scale_offset.g;
    }

    float* scale_z_stream = primitive.get_particle_system_stream(ParticleSystemStream::GENERATED_SCALE_Z);
    KW_ASSERT(scale_z_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        scale_z_stream[i] = random.rand_float() * m_scale_range.b + m_scale_offset.b;
    }
}

} // namespace kw
