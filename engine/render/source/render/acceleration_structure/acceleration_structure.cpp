#include "render/acceleration_structure/acceleration_structure.h"
#include "render/acceleration_structure/acceleration_structure_primitive.h"

#include <core/debug/assert.h>

namespace kw {

void AccelerationStructure::add(AccelerationStructurePrimitive& primitive) {
    KW_ASSERT(primitive.m_acceleration_structure == nullptr);

    primitive.m_acceleration_structure = this;
}

void AccelerationStructure::remove(AccelerationStructurePrimitive& primitive) {
    KW_ASSERT(primitive.m_acceleration_structure == this);

    primitive.m_acceleration_structure = nullptr;
}

} // namespace kw
