#pragma once

#include "render/acceleration_structure/acceleration_structure.h"

#include <core/containers/unique_ptr.h>

#include <shared_mutex>

namespace kw {

struct OctreeNode {
    explicit OctreeNode(MemoryResource& persistent_memory_resource);

    UniquePtr<OctreeNode> children[8];
    Vector<AccelerationStructurePrimitive*> primitives;
    aabbox bounds;
};

// O(logN) `add`, O(logN) `remove`, O(logN) `update`, O(logN) `query`.
class OctreeAccelerationStructure : public AccelerationStructure, private OctreeNode {
public:
    explicit OctreeAccelerationStructure(MemoryResource& persistent_memory_resource,
                                         const float3& center = float3(),
                                         const float3& extent = float3(256.f),
                                         uint32_t max_depth = 6);

    void add(AccelerationStructurePrimitive& primitive) override;
    void remove(AccelerationStructurePrimitive& primitive) override;
    void update(AccelerationStructurePrimitive& primitive) override;

    Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const aabbox& bounds) const override;
    Vector<AccelerationStructurePrimitive*> query(MemoryResource& memory_resource, const frustum& frustum) const override;

    size_t get_count() const override;

private:
    OctreeNode& find_node(const aabbox& bounds, OctreeNode& node, uint32_t depth = 0);

    template <typename Bounds>
    void collect_primitives(const OctreeNode& node, const Bounds& bounds, Vector<AccelerationStructurePrimitive*>& output) const;

    MemoryResource& m_memory_resource;
    uint32_t m_max_depth;
    uint32_t m_count;
    mutable std::shared_mutex m_shared_mutex;
};

} // namespace kw
