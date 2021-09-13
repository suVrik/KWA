#include "render/scene/render_scene.h"
#include "render/acceleration_structure/acceleration_structure.h"
#include "render/animation/animated_geometry_primitive.h"
#include "render/animation/animation_player.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/light_primitive.h"
#include "render/particles/particle_system_player.h"
#include "render/particles/particle_system_primitive.h"
#include "render/reflection_probe/reflection_probe_manager.h"
#include "render/reflection_probe/reflection_probe_primitive.h"

#include <core/debug/assert.h>

namespace kw {

RenderScene::RenderScene(const RenderSceneDescriptor& descriptor)
    : Scene(SceneDescriptor{ descriptor.persistent_memory_resource, descriptor.transient_memory_resource })
    , m_animation_player(*descriptor.animation_player)
    , m_particle_system_player(*descriptor.particle_system_player)
    , m_reflection_probe_manager(*descriptor.reflection_probe_manager)
    , m_geometry_acceleration_structure(*descriptor.geometry_acceleration_structure)
    , m_light_acceleration_structure(*descriptor.light_acceleration_structure)
    , m_particle_system_acceleration_structure(*descriptor.particle_system_acceleration_structure)
    , m_reflection_probe_acceleration_structure(*descriptor.reflection_probe_acceleration_structure)
{
    KW_ASSERT(descriptor.animation_player != nullptr);
    KW_ASSERT(descriptor.particle_system_player != nullptr);
    KW_ASSERT(descriptor.reflection_probe_manager != nullptr);
    KW_ASSERT(descriptor.geometry_acceleration_structure != nullptr);
    KW_ASSERT(descriptor.light_acceleration_structure != nullptr);
    KW_ASSERT(descriptor.particle_system_acceleration_structure != nullptr);
    KW_ASSERT(descriptor.reflection_probe_acceleration_structure != nullptr);
}

Vector<GeometryPrimitive*> RenderScene::query_geometry(const aabbox& bounds) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_geometry_acceleration_structure.query(m_transient_memory_resource, bounds);
    Vector<GeometryPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<GeometryPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<GeometryPrimitive*> RenderScene::query_geometry(const frustum& frustum) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_geometry_acceleration_structure.query(m_transient_memory_resource, frustum);
    Vector<GeometryPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<GeometryPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<LightPrimitive*> RenderScene::query_lights(const aabbox& bounds) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_light_acceleration_structure.query(m_transient_memory_resource, bounds);
    Vector<LightPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<LightPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<LightPrimitive*> RenderScene::query_lights(const frustum& frustum) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_light_acceleration_structure.query(m_transient_memory_resource, frustum);
    Vector<LightPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<LightPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<ParticleSystemPrimitive*> RenderScene::query_particle_systems(const aabbox& bounds) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_particle_system_acceleration_structure.query(m_transient_memory_resource, bounds);
    Vector<ParticleSystemPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<ParticleSystemPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<ParticleSystemPrimitive*> RenderScene::query_particle_systems(const frustum& frustum) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_particle_system_acceleration_structure.query(m_transient_memory_resource, frustum);
    Vector<ParticleSystemPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<ParticleSystemPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<ReflectionProbePrimitive*> RenderScene::query_reflection_probes(const aabbox& bounds) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_reflection_probe_acceleration_structure.query(m_transient_memory_resource, bounds);
    Vector<ReflectionProbePrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<ReflectionProbePrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<ReflectionProbePrimitive*> RenderScene::query_reflection_probes(const frustum& frustum) const {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_reflection_probe_acceleration_structure.query(m_transient_memory_resource, frustum);
    Vector<ReflectionProbePrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<ReflectionProbePrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

void RenderScene::child_added(Primitive& primitive) {
    if (GeometryPrimitive* geometry_primitive = dynamic_cast<GeometryPrimitive*>(&primitive)) {
        if (AnimatedGeometryPrimitive* animated_geometry_primitive = dynamic_cast<AnimatedGeometryPrimitive*>(geometry_primitive)) {
            m_animation_player.add(*animated_geometry_primitive);
        }
        m_geometry_acceleration_structure.add(*geometry_primitive);
    } else if (LightPrimitive* light_primitive = dynamic_cast<LightPrimitive*>(&primitive)) {
        m_light_acceleration_structure.add(*light_primitive);
    } else if (ParticleSystemPrimitive* particle_system_primitive = dynamic_cast<ParticleSystemPrimitive*>(&primitive)) {
        m_particle_system_player.add(*particle_system_primitive);
        m_particle_system_acceleration_structure.add(*particle_system_primitive);
    } else if (ReflectionProbePrimitive* reflection_probe_primitive = dynamic_cast<ReflectionProbePrimitive*>(&primitive)) {
        m_reflection_probe_manager.add(*reflection_probe_primitive);
        m_reflection_probe_acceleration_structure.add(*reflection_probe_primitive);
    }
}

} // namespace kw
