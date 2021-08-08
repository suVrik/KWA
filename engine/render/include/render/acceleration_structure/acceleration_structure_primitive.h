#pragma once

#include "render/scene/primitive.h"

#include <core/math/aabbox.h>

namespace kw {

class aabbox;
class AccelerationStructure;

class AccelerationStructurePrimitive : public Primitive {
public:
    explicit AccelerationStructurePrimitive(const transform& local_transform = transform());
    AccelerationStructurePrimitive(const AccelerationStructurePrimitive& other);
    ~AccelerationStructurePrimitive() override;
    AccelerationStructurePrimitive& operator=(const AccelerationStructurePrimitive& other);

    // Acceleration structure is set from AccelerationStructure's `add` method.
    AccelerationStructure* get_acceleration_structure() const;
    
    // Bounds are defined by global transform, geometry, light radius, etc.
    const aabbox& get_bounds() const;

    // The global counter is incremented on each primitive's change (transform, geometry, radius, etc.).
    // This counter is a value of global counter at last primitive's change. If you wish to check whether
    // the shadow map or reflection probe have changed, check whether the max of the counter of the
    // primitives they render has changed.
    uint64_t get_counter() const;

protected:
    void global_transform_updated() override;

    // Children are responsible for setting bounds.
    aabbox m_bounds;

    // Children are responsible for updating counter.
    uint64_t m_counter;

private:
    AccelerationStructure* m_acceleration_structure;
    void* m_node;

    friend class LinearAccelerationStructure;
    friend class OctreeAccelerationStructure;
    friend class QuadtreeAccelerationStructure;
};

} // namespace kw
