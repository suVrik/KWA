#pragma once

#include "render/acceleration_structure/acceleration_structure.h"

#include <shared_mutex>

namespace kw {

// O(1) `add`, O(n) `remove`, no-op `update`, O(n) `query`.
class LinearAccelerationStructure : public AccelerationStructure {
public:
    LinearAccelerationStructure(MemoryResource& persistent_memory_resource);

    void add(AccelerationStructurePrimitive& primitive) override;
    void remove(AccelerationStructurePrimitive& primitive) override;
    void update(AccelerationStructurePrimitive& primitive) override;

    Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const aabbox& bounds) const override;
    Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const frustum& frustum) const override;

    size_t get_count() const override;

private:
    Vector<AccelerationStructurePrimitive*> m_primitives;
    mutable std::shared_mutex m_shared_mutex;
};

} // namespace kw
