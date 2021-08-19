#include "render/particles/generators/color_particle_system_generator.h"
#include "render/particles/particle_system_primitive.h"
#include "render/particles/particle_system_random.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemGenerator* ColorParticleSystemGenerator::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
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

    return memory_resource.construct<ColorParticleSystemGenerator>(min, max);
}

ColorParticleSystemGenerator::ColorParticleSystemGenerator(const float3& min_color, const float3& max_color)
    : m_color_range(max_color - min_color)
    , m_color_offset(min_color)
{
}

void ColorParticleSystemGenerator::generate(ParticleSystemPrimitive& primitive, size_t begin_index, size_t end_index) const {
    ParticleSystemRandom& random = ParticleSystemRandom::instance();

    float* color_r_stream = primitive.get_particle_system_stream(ParticleSystemStream::COLOR_R);
    KW_ASSERT(color_r_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        color_r_stream[i] = random.rand_float() * m_color_range.r + m_color_offset.r;
    }

    float* color_g_stream = primitive.get_particle_system_stream(ParticleSystemStream::COLOR_G);
    KW_ASSERT(color_g_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        color_g_stream[i] = random.rand_float() * m_color_range.g + m_color_offset.g;
    }

    float* color_b_stream = primitive.get_particle_system_stream(ParticleSystemStream::COLOR_B);
    KW_ASSERT(color_b_stream != nullptr);

    for (size_t i = begin_index; i < end_index; i++) {
        color_b_stream[i] = random.rand_float() * m_color_range.b + m_color_offset.b;
    }
}

ParticleSystemStreamMask ColorParticleSystemGenerator::get_stream_mask() const {
    return ParticleSystemStreamMask::COLOR_R | ParticleSystemStreamMask::COLOR_G | ParticleSystemStreamMask::COLOR_B;
}

} // namespace kw
