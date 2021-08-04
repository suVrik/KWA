#include "render/acceleration_structure/linear_acceleration_structure.h"
#include "render/acceleration_structure/acceleration_structure_primitive.h"
#include "render/scene/primitive.h"

#include <core/debug/assert.h>

namespace kw {

LinearAccelerationStructure::LinearAccelerationStructure(MemoryResource& persistent_memory_resource)
    : m_primitives(persistent_memory_resource)
{
    // Avoid first 8 reallocations.
    m_primitives.reserve(128);
}

void LinearAccelerationStructure::add(AccelerationStructurePrimitive& primitive) {
    std::lock_guard lock_guard(m_shared_mutex);

    KW_ASSERT(primitive.m_acceleration_structure == nullptr);
    primitive.m_acceleration_structure = this;

    KW_ASSERT(std::find(m_primitives.begin(), m_primitives.end(), &primitive) == m_primitives.end());

    m_primitives.push_back(&primitive);
}

void LinearAccelerationStructure::remove(AccelerationStructurePrimitive& primitive) {
    std::lock_guard lock_guard(m_shared_mutex);

    KW_ASSERT(primitive.m_acceleration_structure == static_cast<AccelerationStructure*>(this));
    primitive.m_acceleration_structure = nullptr;

    auto it = std::find(m_primitives.begin(), m_primitives.end(), &primitive);
    KW_ASSERT(it != m_primitives.end());

    m_primitives.erase(it);
}

void LinearAccelerationStructure::update(AccelerationStructurePrimitive& primitive) {
    // No-op.
}

Vector<AccelerationStructurePrimitive*> LinearAccelerationStructure::query(MemoryResource& memory_resource, const aabbox& bounds) const {
    std::shared_lock shared_lock(m_shared_mutex);

    Vector<AccelerationStructurePrimitive*> result(memory_resource);
    result.reserve(64);

    for (AccelerationStructurePrimitive* primitive : m_primitives) {
        if (intersect(primitive->get_bounds(), bounds)) {
            result.push_back(primitive);
        }
    }
    
    return result;
}

Vector<AccelerationStructurePrimitive*> LinearAccelerationStructure::query(MemoryResource& memory_resource, const frustum& frustum) const {
    std::shared_lock shared_lock(m_shared_mutex);

    Vector<AccelerationStructurePrimitive*> result(memory_resource);
    result.reserve(64);
    
    for (AccelerationStructurePrimitive* primitive : m_primitives) {
        if (intersect(primitive->get_bounds(), frustum)) {
            result.push_back(primitive);
        }
    }
    
    return result;
}

size_t LinearAccelerationStructure::get_count() const {
    std::shared_lock shared_lock(m_shared_mutex);

    return m_primitives.size();
}

} // namespace kw
