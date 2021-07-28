#include "render/scene/scene.h"
#include "render/acceleration_structure/linear_acceleration_structure.h"
#include "render/geometry/geometry_primitive.h"
#include "render/light/light_primitive.h"

#include <core/debug/assert.h>

namespace kw {

Scene::Scene(MemoryResource& persistent_memory_resource, MemoryResource& transient_memory_resource)
    : ContainerPrimitive(persistent_memory_resource)
    , m_persistent_memory_resource(persistent_memory_resource)
    , m_transient_memory_resource(transient_memory_resource)
    , m_geometry_acceleration_structure(static_pointer_cast<AccelerationStructure>(allocate_unique<LinearAccelerationStructure>(persistent_memory_resource, persistent_memory_resource)))
    , m_light_acceleration_structure(static_pointer_cast<AccelerationStructure>(allocate_unique<LinearAccelerationStructure>(persistent_memory_resource, persistent_memory_resource)))
{
}

Scene::~Scene() = default;

Vector<GeometryPrimitive*> Scene::query_geometry(const aabbox& bounds) {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_geometry_acceleration_structure->query(m_transient_memory_resource, bounds);
    Vector<GeometryPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<GeometryPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<GeometryPrimitive*> Scene::query_geometry(const frustum& frustum) {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_geometry_acceleration_structure->query(m_transient_memory_resource, frustum);
    Vector<GeometryPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<GeometryPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<LightPrimitive*> Scene::query_lights(const aabbox& bounds) {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_light_acceleration_structure->query(m_transient_memory_resource, bounds);
    Vector<LightPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<LightPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

Vector<LightPrimitive*> Scene::query_lights(const frustum& frustum) {
    Vector<AccelerationStructurePrimitive*> acceleration_structure_primitives = m_light_acceleration_structure->query(m_transient_memory_resource, frustum);
    Vector<LightPrimitive*> result(acceleration_structure_primitives.size(), m_transient_memory_resource);
    for (size_t i = 0; i < acceleration_structure_primitives.size(); i++) {
        result[i] = static_cast<LightPrimitive*>(acceleration_structure_primitives[i]);
    }
    return result;
}

void Scene::child_added(Primitive& primitive) {
    if (GeometryPrimitive* geometry_primitive = dynamic_cast<GeometryPrimitive*>(&primitive)) {
        m_geometry_acceleration_structure->add(*geometry_primitive);
    } else if (LightPrimitive* light_primitive = dynamic_cast<LightPrimitive*>(&primitive)) {
        m_light_acceleration_structure->add(*light_primitive);
    } else if (ContainerPrimitive* container_primitive = dynamic_cast<ContainerPrimitive*>(&primitive)) {
        add_container_primitive(*container_primitive);
    } else {
        KW_ASSERT(false, "Invalid primitive type.");
    }
}

void Scene::child_removed(Primitive& primitive) {
    if (GeometryPrimitive* geometry_primitive = dynamic_cast<GeometryPrimitive*>(&primitive)) {
        m_geometry_acceleration_structure->remove(*geometry_primitive);
    } else if (LightPrimitive* light_primitive = dynamic_cast<LightPrimitive*>(&primitive)) {
        m_light_acceleration_structure->remove(*light_primitive);
    } else if (ContainerPrimitive* container_primitive = dynamic_cast<ContainerPrimitive*>(&primitive)) {
        remove_container_primitive(*container_primitive);
    } else {
        KW_ASSERT(false, "Invalid primitive type.");
    }
}

void Scene::add_container_primitive(ContainerPrimitive& container_primitive) {
    const Vector<Primitive*>& children = container_primitive.get_children();
    for (Primitive* primitive : children) {
        child_added(*primitive);
    }
}

void Scene::remove_container_primitive(ContainerPrimitive& container_primitive) {
    const Vector<Primitive*>& children = container_primitive.get_children();
    for (Primitive* primitive : children) {
        child_removed(*primitive);
    }
}

} // namespace kw
