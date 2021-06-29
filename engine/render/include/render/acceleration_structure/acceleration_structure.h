#pragma once

#include <core/containers/vector.h>
#include <core/math/aabbox.h>
#include <core/math/frustum.h>

namespace kw {

class AccelerationStructurePrimitive;

class AccelerationStructure {
public:
    virtual void add(AccelerationStructurePrimitive& primitive) = 0;
    virtual void remove(AccelerationStructurePrimitive& primitive) = 0;
    virtual void update(AccelerationStructurePrimitive& primitive) = 0;

    virtual Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const aabbox& bounds) const = 0;
    virtual Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const frustum& frustum) const = 0;

    virtual size_t get_count() const = 0;
};

} // namespace kw
