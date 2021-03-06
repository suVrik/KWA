#include "render/acceleration_structure/acceleration_structure_primitive.h"
#include "render/acceleration_structure/acceleration_structure.h"

#include <core/debug/assert.h>

#include <atomic>

namespace kw {

// Not static. Other source files can use it with `extern` specifier.
std::atomic_uint64_t acceleration_structure_counter;

AccelerationStructurePrimitive::AccelerationStructurePrimitive(const transform& local_transform)
    : Primitive(local_transform)
    , m_counter(++acceleration_structure_counter)
    , m_acceleration_structure(nullptr)
    , m_node(nullptr)
{
}

AccelerationStructurePrimitive::AccelerationStructurePrimitive(const AccelerationStructurePrimitive& other)
    : Primitive(other)
    , m_bounds(other.m_bounds)
    , m_counter(++acceleration_structure_counter)
    , m_acceleration_structure(nullptr)
    , m_node(nullptr)
{
    KW_ASSERT(
        other.m_acceleration_structure == nullptr,
        "Copying acceleration structure primitives assigned to some acceleration structure is not allowed."
    );
}

AccelerationStructurePrimitive::~AccelerationStructurePrimitive() {
    if (m_acceleration_structure != nullptr) {
        m_acceleration_structure->remove(*this);
    }
}

AccelerationStructurePrimitive& AccelerationStructurePrimitive::operator=(const AccelerationStructurePrimitive& other) {
    Primitive::operator=(other);

    if (m_acceleration_structure != nullptr) {
        m_acceleration_structure->remove(*this);
    }

    m_bounds = other.m_bounds;
    m_counter = ++acceleration_structure_counter;
    m_acceleration_structure = nullptr;
    m_node = nullptr;

    KW_ASSERT(
        other.m_acceleration_structure == nullptr,
        "Copying acceleration structure primitives assigned to some acceleration structure is not allowed."
    );

    return *this;
}

AccelerationStructure* AccelerationStructurePrimitive::get_acceleration_structure() const {
    return m_acceleration_structure;
}

const aabbox& AccelerationStructurePrimitive::get_bounds() const {
    return m_bounds;
}

uint64_t AccelerationStructurePrimitive::get_counter() const {
    return m_counter;
}

void AccelerationStructurePrimitive::global_transform_updated() {
    m_counter = ++acceleration_structure_counter;

    if (m_acceleration_structure != nullptr) {
        m_acceleration_structure->update(*this);
    }
}

} // namespace kw
