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

protected:
    void global_transform_updated() override;

    // Children are responsible for setting bounds.
    aabbox m_bounds;

private:
    AccelerationStructure* m_acceleration_structure;
    void* m_node;

    friend class LinearAccelerationStructure;
    friend class OctreeAccelerationStructure;
    friend class QuadtreeAccelerationStructure;
};

} // namespace kw
