#pragma once

#include <core/math/transform.h>

namespace kw {

class ContainerPrimitive;

// Keep in mind that none of the primitives can be accessed from multiple threads at the same time.
class Primitive {
public:
    explicit Primitive(const transform& local_transform = transform());

    // When primitive is deleted, it is automatically removed from parent container.
    virtual ~Primitive();

    // Parent is set from ContainerPrimitive's `add_child` method.
    ContainerPrimitive* get_parent() const {
        return m_parent;
    }

    const transform& get_local_transform() const {
        return m_local_transform;
    }

    const float3& get_local_translation() const {
        return m_local_transform.translation;
    }

    const quaternion& get_local_rotation() const {
        return m_local_transform.rotation;
    }

    const float3& get_local_scale() const {
        return m_local_transform.scale;
    }

    // Updates children global transform and bounds too.
    void set_local_transform(const transform& transform);
    void set_local_translation(const float3& translation);
    void set_local_rotation(const quaternion& rotation);
    void set_local_scale(const float3& scale);

    const transform& get_global_transform() const {
        return m_global_transform;
    }

    const float3& get_global_translation() const {
        return m_global_transform.translation;
    }

    const quaternion& get_global_rotation() const {
        return m_global_transform.rotation;
    }

    const float3& get_global_scale() const {
        return m_global_transform.scale;
    }

    // Updates children global transform and bounds too.
    void set_global_transform(const transform& transform);
    void set_global_translation(const float3& translation);
    void set_global_rotation(const quaternion& rotation);
    void set_global_scale(const float3& scale);

protected:
    // Acceleration structure primitives must update their bounds, container primitives must propagate global transform.
    virtual void global_transform_updated();
    
    ContainerPrimitive* m_parent;

    transform m_local_transform;
    transform m_global_transform;

    friend class ContainerPrimitive;
};

} // namespace kw
