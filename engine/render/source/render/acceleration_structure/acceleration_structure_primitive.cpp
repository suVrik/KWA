#include "render/acceleration_structure/acceleration_structure_primitive.h"
#include "render/acceleration_structure/acceleration_structure.h"

namespace kw {

AccelerationStructurePrimitive::AccelerationStructurePrimitive(const transform& local_transform)
    : Primitive(local_transform)
    , m_acceleration_structure(nullptr)
    , m_node(nullptr)
{
}

AccelerationStructurePrimitive::~AccelerationStructurePrimitive() {
    if (m_acceleration_structure != nullptr) {
        m_acceleration_structure->remove(*this);
    }
}

} // namespace kw
