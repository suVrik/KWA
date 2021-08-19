#include "render/particles/updaters/alpha_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/over_lifetime_particle_system_updater_impl.h"
#include "render/particles/particle_system_primitive.h"

#include <core/error.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemUpdater* AlphaOverLifetimeParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
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

    Vector<float> outputs(memory_resource);
    inputs.reserve(outputs_node.get_size());

    for (const UniquePtr<MarkdownNode>& it : outputs_node) {
        outputs.push_back(it->as<NumberNode>().get_value());
    }

    return memory_resource.construct<AlphaOverLifetimeParticleSystemUpdater>(std::move(inputs), std::move(outputs));
}

AlphaOverLifetimeParticleSystemUpdater::AlphaOverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<float>&& outputs)
    : OverLifetimeParticleSystemUpdater(std::move(inputs), std::move(outputs))
{
}

void AlphaOverLifetimeParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    update_stream<ParticleSystemStream::COLOR_A, 0>(primitive);
}

ParticleSystemStreamMask AlphaOverLifetimeParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::COLOR_A | ParticleSystemStreamMask::TOTAL_LIFETIME | ParticleSystemStreamMask::CURRENT_LIFETIME;
}

} // namespace kw
