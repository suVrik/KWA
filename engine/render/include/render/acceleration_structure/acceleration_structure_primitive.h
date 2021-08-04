#pragma once

#include "render/scene/primitive.h"

namespace kw {

class aabbox;
class AccelerationStructure;

class AccelerationStructurePrimitive : public Primitive {
public:
    explicit AccelerationStructurePrimitive(const transform& local_transform = transform());
    ~AccelerationStructurePrimitive() override;

    // Acceleration structure is set from AccelerationStructure's `add` method.
    AccelerationStructure* get_acceleration_structure() const {
        return m_acceleration_structure;
    }

    virtual const aabbox& get_bounds() const = 0;

protected:
    // TODO: Call acceleration structure's `update` from `global_transform_updated`.

private:
    AccelerationStructure* m_acceleration_structure;
    void* m_node;

    friend class LinearAccelerationStructure;
    friend class OctreeAccelerationStructure;
    friend class QuadtreeAccelerationStructure;
};

} // namespace kw
