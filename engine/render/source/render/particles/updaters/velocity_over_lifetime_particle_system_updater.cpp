#include "render/particles/updaters/velocity_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/over_lifetime_particle_system_updater_impl.h"
#include "render/particles/particle_system_primitive.h"

#include <core/error.h>
#include <core/io/markdown.h>
#include <core/io/markdown_utils.h>

namespace kw {

ParticleSystemUpdater* VelocityOverLifetimeParticleSystemUpdater::create_from_markdown(MemoryResource& memory_resource, ObjectNode& node) {
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
        outputs.push_back(MarkdownUtils::float3_from_markdown(*it));
    }

    return memory_resource.construct<VelocityOverLifetimeParticleSystemUpdater>(std::move(inputs), std::move(outputs));
}

VelocityOverLifetimeParticleSystemUpdater::VelocityOverLifetimeParticleSystemUpdater(Vector<float>&& inputs, Vector<float3>&& outputs)
    : OverLifetimeParticleSystemUpdater(std::move(inputs), std::move(outputs))
{
}

void VelocityOverLifetimeParticleSystemUpdater::update(ParticleSystemPrimitive& primitive, float elapsed_time) const {
    update_stream<ParticleSystemStream::VELOCITY_X, 0>(primitive);
    update_stream<ParticleSystemStream::VELOCITY_Y, 1>(primitive);
    update_stream<ParticleSystemStream::VELOCITY_Z, 2>(primitive);
}

ParticleSystemStreamMask VelocityOverLifetimeParticleSystemUpdater::get_stream_mask() const {
    return ParticleSystemStreamMask::VELOCITY_X | ParticleSystemStreamMask::VELOCITY_Y | ParticleSystemStreamMask::VELOCITY_Z |
           ParticleSystemStreamMask::TOTAL_LIFETIME | ParticleSystemStreamMask::CURRENT_LIFETIME;
}

} // namespace kw
