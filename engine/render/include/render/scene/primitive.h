#pragma once

#include <core/math/transform.h>

namespace kw {

class ContainerPrimitive;

// Keep in mind that none of the primitives can be accessed from multiple threads at the same time.
class Primitive {
public:
    explicit Primitive(const transform& local_transform = transform());
    Primitive(const Primitive& other);
    virtual ~Primitive();
    Primitive& operator=(const Primitive& other);

    // Parent is set from ContainerPrimitive's `add_child` method.
    ContainerPrimitive* get_parent() const;

    const transform& get_local_transform() const;
    void set_local_transform(const transform& transform);

    const float3& get_local_translation() const;
    void set_local_translation(const float3& translation);

    const quaternion& get_local_rotation() const;
    void set_local_rotation(const quaternion& rotation);

    const float3& get_local_scale() const;
    void set_local_scale(const float3& scale);

    const transform& get_global_transform() const;
    void set_global_transform(const transform& transform);

    const float3& get_global_translation() const;
    void set_global_translation(const float3& translation);

    const quaternion& get_global_rotation() const;
    void set_global_rotation(const quaternion& rotation);

    const float3& get_global_scale() const;
    void set_global_scale(const float3& scale);

protected:
    // Acceleration structure primitives must update their bounds, container primitives must propagate global transform.
    virtual void global_transform_updated();
    
private:
    ContainerPrimitive* m_parent;

    transform m_local_transform;
    transform m_global_transform;

    // Friendship is needed to access `m_parent`.
    friend class ContainerPrimitive;
};

} // namespace kw
