#include "render/scene/primitive.h"
#include "render/container/container_primitive.h"

namespace kw {

Primitive::Primitive()
    : m_parent(nullptr)
{
}

Primitive::~Primitive() {
    if (m_parent != nullptr) {
        m_parent->remove_child(this);
    }
}

void Primitive::set_local_transform(const transform& transform) {
    m_local_transform = transform;

    if (m_parent != nullptr) {
        m_global_transform = m_local_transform * m_parent->get_global_transform();
    } else {
        m_global_transform = m_local_transform;
    }

    // Render primitives must update their bounds, container primitives must propagate global transform.
    global_transform_updated();
}

void Primitive::set_global_transform(const transform& transform) {
    m_global_transform = transform;

    if (m_parent != nullptr) {
        m_local_transform = m_global_transform * inverse(m_parent->get_global_transform());
    } else {
        m_local_transform = m_global_transform;
    }

    // Render primitives must update their bounds, container primitives must propagate global transform.
    global_transform_updated();
}

void Primitive::global_transform_updated() {
    // Object is being destroyed and virtual method is called.
}

} // namespace kw
