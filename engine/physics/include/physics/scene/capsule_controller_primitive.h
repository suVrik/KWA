#pragma once

#include "physics/scene/controller_primitive.h"

namespace kw {

class ObjectNode;
class PrimitiveReflection;

class CapsuleControllerPrimitive : public ControllerPrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    explicit CapsuleControllerPrimitive(float radius = 0.5f,
                                        float height = 0.5f,
                                        float step_offset = 0.5f,
                                        const transform& local_transform = transform());
    ~CapsuleControllerPrimitive() override;

    float get_radius() const;
    void set_radius(float value);

    float get_height() const;
    void set_height(float value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

private:
    float m_radius;
    float m_height;
};

} // namespace kw
