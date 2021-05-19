#pragma once

#include <core/containers/vector.h>
#include <core/math/aabbox.h>
#include <core/math/frustum.h>

namespace kw {

class AccelerationStructurePrimitive;

class AccelerationStructure {
public:
    virtual void add(AccelerationStructurePrimitive& primitive);
    virtual void remove(AccelerationStructurePrimitive& primitive);

    // Primitive must have old bounds set at this point. Once this method is called, new bounds must be set manually.
    virtual void update(AccelerationStructurePrimitive& primitive, const aabbox& new_bounds) = 0;

    virtual Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const aabbox& bounds) const = 0;
    virtual Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const frustum& frustum) const = 0;

    virtual size_t get_count() const = 0;
};

} // namespace kw
