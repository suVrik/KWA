#pragma once

#include <core/math/transform.h>

namespace kw {

class ContainerPrimitive;

// Keep in mind that none of the primitives can be accessed from multiple threads at the same time.
class Primitive {
public:
    Primitive();

    // When primitive is deleted, it is automatically removed from parent container.
    virtual ~Primitive();

    // Parent is set from ContainerPrimitive's `add_child` method.
    ContainerPrimitive* get_parent() const {
        return m_parent;
    }

    const transform& get_local_transform() const {
        return m_local_transform;
    }

    // Updates children global transform and bounds too.
    void set_local_transform(const transform& transform);

    const transform& get_global_transform() const {
        return m_global_transform;
    }

    // Updates children global transform and bounds too.
    void set_global_transform(const transform& transform);

protected:
    // Acceleration structure primitives must update their bounds, container primitives must propagate global transform.
    virtual void global_transform_updated();
    
    ContainerPrimitive* m_parent;

    transform m_local_transform;
    transform m_global_transform;

    friend class ContainerPrimitive;
};

} // namespace kw
