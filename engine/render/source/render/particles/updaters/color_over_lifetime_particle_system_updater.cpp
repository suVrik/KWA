#include "render/particles/updaters/color_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/over_lifetime_particle_system_updater_impl.h"
#include "render/particles/particle_system_primitive.h"

#include <core/error.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemUpdater* ColorOverLifetimeParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
    ArrayNode& inputs_node = node["inputs"].as<ArrayNode>();
    KW_ERROR(inputs_node.get_size() >= 2, "Invalid inputs.");

    Vector<float> inputs(memory_resource);
    inputs.reserve(inputs_node.get_size());

    for (const UniquePtr<MarkdownNode>& it : inputs_node) {
        inputs.push_back(it->as<NumberNode>().get_value());
    }

    KW_ERROR(inputs.front() == 0.f, "Invalid inputs.");
    KW_ERROR(inputs.back() == 1.f, "Invalid inputs.");

    ArrayNode& outputs_node = node["outputs"].as<ArrayNode>();
    KW_ERROR(outputs_node.get_size() == inputs_node.get_size(), "Invalid outputs.");

    Vector<float3> outputs(memory_resource);
    inputs.reserve(outputs_node.get_size());

    for (const UniquePtr<MarkdownNode>& it : outputs_node) {
        ArrayNode& output_node = it->as<ArrayNode>();
        KW_ERROR(output_node.get_size() == 3, "Invalid outputs.");
        
        float3 output(static_cast<float>(output_node[0].as<NumberNode>().get_value()),
                      static_cast<float>(output_node[1].as<NumberNode>().get_value()),
                      static_cast<float>(output_node[2].as<NumberNode>().get_value()));

        outputs.push_back(output);
    }

    return memory_resource.construct<ColorOverLifetimeParticleSystemUpdater>(std::move(inputs), std::move(outputs));
}

ColorOverLifetimeParticleSystemUpdater::ColorOverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<float3>&& outputs)
    : OverLifetimeParticleSystemUpdater(std::move(inputs), std::move(outputs))
{
}

void ColorOverLifetimeParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    update_stream<ParticleSystemStream::COLOR_R, 0>(primitive);
    update_stream<ParticleSystemStream::COLOR_G, 1>(primitive);
    update_stream<ParticleSystemStream::COLOR_B, 2>(primitive);
}

ParticleSystemStreamMask ColorOverLifetimeParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::COLOR_R | ParticleSystemStreamMask::COLOR_G | ParticleSystemStreamMask::COLOR_B |
           ParticleSystemStreamMask::TOTAL_LIFETIME | ParticleSystemStreamMask::CURRENT_LIFETIME;
}

} // namespace kw
