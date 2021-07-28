#include "render/container/container_primitive.h"

#include <core/debug/assert.h>

namespace kw {

ContainerPrimitive::ContainerPrimitive(MemoryResource& persistent_memory_resource)
    : m_children(persistent_memory_resource)
{
}

ContainerPrimitive::~ContainerPrimitive() {
    for (Primitive* child : m_children) {
        child->m_parent = nullptr;

        // Primitive leaves this container's system coordinate.
        child->m_global_transform = child->m_local_transform;

        // Update primitive's global transform and bounds recursively.
        child->global_transform_updated();
    }
}

void ContainerPrimitive::add_child(Primitive& primitive) {
    KW_ASSERT(primitive.m_parent == nullptr, "Primitive already has a parent.");
    KW_ASSERT(std::find(m_children.begin(), m_children.end(), &primitive) == m_children.end());

    m_children.push_back(&primitive);

    primitive.m_parent = this;

    // Primitive joins this container's system coordinate.
    primitive.m_global_transform = primitive.m_local_transform * m_global_transform;

    // Update primitive's global transform and bounds recursively.
    primitive.global_transform_updated();

    // Notify all parents about added child after it's added.
    child_added(primitive);
}

void ContainerPrimitive::add_children(const Vector<Primitive*>& children) {
    m_children.reserve(m_children.size() + children.size());

    for (Primitive* primitive : children) {
        KW_ASSERT(primitive != nullptr, "Invalid primitive.");

        add_child(*primitive);
    }
}

void ContainerPrimitive::remove_child(Primitive& primitive) {
    KW_ASSERT(primitive.m_parent == this, "Invalid primitive.");

    auto it = std::find(m_children.begin(), m_children.end(), &primitive);
    KW_ASSERT(it != m_children.end());

    // Notify all parents about removed child before it's removed.
    child_removed(primitive);

    m_children.erase(it);

    primitive.m_parent = nullptr;

    // Primitive leaves this container's system coordinate.
    primitive.m_global_transform = primitive.m_local_transform;

    // Update primitive's global transform and bounds recursively.
    primitive.global_transform_updated();
}

void ContainerPrimitive::global_transform_updated() {
    // Global transform had been updated outside.

    for (Primitive* child : m_children) {
        // Update child's global transform too.
        child->m_global_transform = child->m_local_transform * m_global_transform;

        // Render primitives must update their bounds, container primitives must propagate global transform.
        child->global_transform_updated();
    }
}

void ContainerPrimitive::child_added(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_added(primitive);
    }
}

void ContainerPrimitive::child_removed(Primitive& primitive) {
    if (m_parent != nullptr) {
        m_parent->child_removed(primitive);
    }
}

} // namespace kw
