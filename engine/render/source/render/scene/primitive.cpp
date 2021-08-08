#include "render/scene/primitive.h"
#include "render/container/container_primitive.h"

#include <core/debug/assert.h>

namespace kw {

Primitive::Primitive(const transform& local_transform)
    : m_parent(nullptr)
    , m_local_transform(local_transform)
    , m_global_transform(local_transform)
{
}

Primitive::Primitive(const Primitive& other)
    : m_parent(nullptr)
    , m_local_transform(other.m_local_transform)
    , m_global_transform(other.m_local_transform)
{
    KW_ASSERT(
        other.m_parent == nullptr,
        "Copying primitives with a parent is not allowed."
    );
}

Primitive::~Primitive() {
    if (m_parent != nullptr) {
        m_parent->remove_child(*this);
    }
}

Primitive& Primitive::operator=(const Primitive& other) {
    if (m_parent != nullptr) {
        m_parent->remove_child(*this);
    }

    m_parent = nullptr;
    m_local_transform = other.m_local_transform;
    m_global_transform = other.m_local_transform;

    KW_ASSERT(
        other.m_parent == nullptr,
        "Copying primitives with a parent is not allowed."
    );

    return *this;
}

ContainerPrimitive* Primitive::get_parent() const {
    return m_parent;
}

const transform& Primitive::get_local_transform() const {
    return m_local_transform;
}

void Primitive::set_local_transform(const transform& transform) {
    if (m_local_transform != transform) {
        m_local_transform = transform;

        if (m_parent != nullptr) {
            m_global_transform = m_local_transform * m_parent->get_global_transform();
        } else {
            m_global_transform = m_local_transform;
        }

        // Render primitives must update their bounds, container primitives must propagate global transform.
        global_transform_updated();
    }
}

const float3& Primitive::get_local_translation() const {
    return m_local_transform.translation;
}

void Primitive::set_local_translation(const float3& translation) {
    set_local_transform(transform(translation, m_local_transform.rotation, m_local_transform.scale));
}

const quaternion& Primitive::get_local_rotation() const {
    return m_local_transform.rotation;
}

void Primitive::set_local_rotation(const quaternion& rotation) {
    set_local_transform(transform(m_local_transform.translation, rotation, m_local_transform.scale));
}

const float3& Primitive::get_local_scale() const {
    return m_local_transform.scale;
}

void Primitive::set_local_scale(const float3& scale) {
    set_local_transform(transform(m_local_transform.translation, m_local_transform.rotation, scale));
}

const transform& Primitive::get_global_transform() const {
    return m_global_transform;
}

void Primitive::set_global_transform(const transform& transform) {
    if (m_global_transform != transform) {
        m_global_transform = transform;

        if (m_parent != nullptr) {
            m_local_transform = m_global_transform * inverse(m_parent->get_global_transform());
        } else {
            m_local_transform = m_global_transform;
        }

        // Render primitives must update their bounds, container primitives must propagate global transform.
        global_transform_updated();
    }
}

const float3& Primitive::get_global_translation() const {
    return m_global_transform.translation;
}

void Primitive::set_global_translation(const float3& translation) {
    set_global_transform(transform(translation, m_global_transform.rotation, m_global_transform.scale));
}

const quaternion& Primitive::get_global_rotation() const {
    return m_global_transform.rotation;
}

void Primitive::set_global_rotation(const quaternion& rotation) {
    set_global_transform(transform(m_global_transform.translation, rotation, m_global_transform.scale));
}

const float3& Primitive::get_global_scale() const {
    return m_global_transform.scale;
}

void Primitive::set_global_scale(const float3& scale) {
    set_global_transform(transform(m_global_transform.translation, m_global_transform.rotation, scale));
}

void Primitive::global_transform_updated() {
    // Object is being destroyed and virtual method is called.
}

} // namespace kw
