#include "render/particles/particle_system_reflection.h"
#include "render/geometry/geometry_manager.h"
#include "render/material/material_manager.h"
#include "render/particles/emitters/over_lifetime_particle_system_emitter.h"
#include "render/particles/generators/alpha_particle_system_generator.h"
#include "render/particles/generators/color_particle_system_generator.h"
#include "render/particles/generators/cylinder_position_particle_system_generator.h"
#include "render/particles/generators/frame_particle_system_generator.h"
#include "render/particles/generators/lifetime_particle_system_generator.h"
#include "render/particles/generators/scale_particle_system_generator.h"
#include "render/particles/generators/velocity_particle_system_generator.h"
#include "render/particles/particle_system.h"
#include "render/particles/updaters/alpha_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/color_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/frame_particle_system_updater.h"
#include "render/particles/updaters/lifetime_particle_system_updater.h"
#include "render/particles/updaters/position_particle_system_updater.h"
#include "render/particles/updaters/scale_by_speed_particle_system_updater.h"
#include "render/particles/updaters/scale_over_lifetime_particle_system_updater.h"
#include "render/particles/updaters/velocity_over_lifetime_particle_system_updater.h"

#include <core/debug/assert.h>
#include <core/error.h>
#include <core/io/markdown.h>

namespace kw {

ParticleSystemReflection& ParticleSystemReflection::instance() {
    static ParticleSystemReflection reflection;
    return reflection;
}

ParticleSystemDescriptor ParticleSystemReflection::create_from_markdown(const ParticleSystemReflectionDescriptor& descriptor) {
    KW_ASSERT(descriptor.particle_system_node != nullptr);
    KW_ASSERT(descriptor.geometry_manager != nullptr);
    KW_ASSERT(descriptor.material_manager != nullptr);
    KW_ASSERT(descriptor.persistent_memory_resource != nullptr);

    ObjectNode& node = *descriptor.particle_system_node;
    MemoryResource& memory_resource = *descriptor.persistent_memory_resource;

    ParticleSystemDescriptor result(*descriptor.persistent_memory_resource);

    result.particle_system_notifier = descriptor.particle_system_notifier;
    result.duration = static_cast<float>(node["duration"].as<NumberNode>().get_value());
    result.loop_count = static_cast<uint32_t>(node["duration"].as<NumberNode>().get_value());
    result.max_particle_count = static_cast<size_t>(node["max_particle_count"].as<NumberNode>().get_value());
    result.geometry = descriptor.geometry_manager->load(node["geometry"].as<StringNode>().get_value().c_str());
    result.material = descriptor.material_manager->load(node["material"].as<StringNode>().get_value().c_str());
    result.spritesheet_x = static_cast<uint32_t>(node["spritesheet_x"].as<NumberNode>().get_value());
    result.spritesheet_y = static_cast<uint32_t>(node["spritesheet_y"].as<NumberNode>().get_value());

    const ObjectNode& max_bounds = node["max_bounds"].as<ObjectNode>();

    const ArrayNode& max_bounds_center = max_bounds["center"].as<ArrayNode>();
    KW_ERROR(max_bounds_center.get_size() == 3, "Invalid center size.");

    result.max_bounds.center = float3(static_cast<float>(max_bounds_center[0].as<NumberNode>().get_value()),
                                      static_cast<float>(max_bounds_center[1].as<NumberNode>().get_value()),
                                      static_cast<float>(max_bounds_center[2].as<NumberNode>().get_value()));

    const ArrayNode& max_bounds_extent = max_bounds["extent"].as<ArrayNode>();
    KW_ERROR(max_bounds_extent.get_size() == 3, "Invalid extent size.");

    result.max_bounds.extent = float3(static_cast<float>(max_bounds_extent[0].as<NumberNode>().get_value()),
                                      static_cast<float>(max_bounds_extent[1].as<NumberNode>().get_value()),
                                      static_cast<float>(max_bounds_extent[2].as<NumberNode>().get_value()));

    const String& shadow_material = node["shadow_material"].as<StringNode>().get_value();
    if (!shadow_material.empty()) {
        result.shadow_material = descriptor.material_manager->load(shadow_material.c_str());
    }

    const String& axes = node["axes"].as<StringNode>().get_value();
    if (axes == "NONE") {
        result.axes = ParticleSystemAxes::NONE;
    } else if (axes == "Y") {
        result.axes = ParticleSystemAxes::Y;
    } else {
        KW_ERROR(axes == "YZ", "Invalid particle system axes.");
        result.axes = ParticleSystemAxes::YZ;
    }

    ObjectNode& emitters_node = node["emitters"].as<ObjectNode>();
    ObjectNode& generators_node = node["generators"].as<ObjectNode>();
    ObjectNode& updaters_node = node["updaters"].as<ObjectNode>();

    size_t emitter_count = 0;
    size_t generator_count = 0;
    size_t updater_count = 0;

    for (const auto& [name, _] : m_emitters) {
        if (emitters_node.find(name.c_str()) != nullptr) {
            emitter_count++;
        }
    }

    for (const auto& [name, _] : m_generators) {
        if (generators_node.find(name.c_str()) != nullptr) {
            generator_count++;
        }
    }

    for (const auto& [name, _] : m_updaters) {
        if (updaters_node.find(name.c_str()) != nullptr) {
            updater_count++;
        }
    }

    result.emitters.reserve(emitter_count);
    result.generators.reserve(generator_count);
    result.updaters.reserve(updater_count);

    for (const auto& [name, callback] : m_emitters) {
        MarkdownNode* child_node = emitters_node.find(name.c_str());
        if (child_node != nullptr) {
            result.emitters.push_back(UniquePtr<ParticleSystemEmitter>(callback(memory_resource, child_node->as<ObjectNode>()), memory_resource));
        }
    }

    for (const auto& [name, callback] : m_generators) {
        MarkdownNode* child_node = generators_node.find(name.c_str());
        if (child_node != nullptr) {
            result.generators.push_back(UniquePtr<ParticleSystemGenerator>(callback(memory_resource, child_node->as<ObjectNode>()), memory_resource));
        }
    }

    for (const auto& [name, callback] : m_updaters) {
        MarkdownNode* child_node = updaters_node.find(name.c_str());
        if (child_node != nullptr) {
            result.updaters.push_back(UniquePtr<ParticleSystemUpdater>(callback(memory_resource, child_node->as<ObjectNode>()), memory_resource));
        }
    }

    return result;
}

#define KW_NAME_AND_CALLBACK(Type) String(#Type, MallocMemoryResource::instance()), &Type::create_from_markdown

ParticleSystemReflection::ParticleSystemReflection()
    : m_emitters(MallocMemoryResource::instance())
    , m_generators(MallocMemoryResource::instance())
    , m_updaters(MallocMemoryResource::instance())
{
    m_emitters.emplace_back(KW_NAME_AND_CALLBACK(OverLifetimeParticleSystemEmitter));

    m_generators.emplace_back(KW_NAME_AND_CALLBACK(AlphaParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(ColorParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(CylinderPositionParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(FrameParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(LifetimeParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(ScaleParticleSystemGenerator));
    m_generators.emplace_back(KW_NAME_AND_CALLBACK(VelocityParticleSystemGenerator));

    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(LifetimeParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(AlphaOverLifetimeParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(ColorOverLifetimeParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(FrameParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(VelocityOverLifetimeParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(PositionParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(ScaleOverLifetimeParticleSystemUpdater));
    m_updaters.emplace_back(KW_NAME_AND_CALLBACK(ScaleBySpeedParticleSystemUpdater));
}

#undef KW_NAME_AND_CALLBACK

} // namespace kw
