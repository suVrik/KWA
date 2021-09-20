#pragma once

#include "render/animation/animated_geometry_primitive.h"

namespace kw {

class Animation;
class ObjectNode;
class PrimitiveReflection;

class SimpleAnimatedGeometryPrimitive : public AnimatedGeometryPrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    SimpleAnimatedGeometryPrimitive(MemoryResource& memory_resource,
                                    SharedPtr<Animation> animation = nullptr,
                                    SharedPtr<Geometry> geometry = nullptr,
                                    SharedPtr<Material> material = nullptr,
                                    SharedPtr<Material> shadow_material = nullptr,
                                    const transform& local_transform = transform());

    const SharedPtr<Animation>& get_animation() const;
    void set_animation(SharedPtr<Animation> animation);

    float get_animation_time() const;
    void set_animation_time(float value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void update_animation(MemoryResource& transient_memory_resource, float elapsed_time) override;

private:
    SharedPtr<Animation> m_animation;
    float m_animation_time;
};

} // namespace kw
