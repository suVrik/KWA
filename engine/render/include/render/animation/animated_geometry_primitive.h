#pragma once

#include "render/geometry/geometry_primitive.h"
#include "render/geometry/skeleton_pose.h"

namespace kw {

class Animation;
class AnimationPlayer;
class ObjectNode;
class PrimitiveReflection;

class AnimatedGeometryPrimitive : public GeometryPrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    AnimatedGeometryPrimitive(MemoryResource& memory_resource,
                              SharedPtr<Animation> animation = nullptr,
                              SharedPtr<Geometry> geometry = nullptr,
                              SharedPtr<Material> material = nullptr,
                              SharedPtr<Material> shadow_material = nullptr,
                              const transform& local_transform = transform());
    AnimatedGeometryPrimitive(const AnimatedGeometryPrimitive& other);
    AnimatedGeometryPrimitive(AnimatedGeometryPrimitive&& other);
    ~AnimatedGeometryPrimitive() override;
    AnimatedGeometryPrimitive& operator=(const AnimatedGeometryPrimitive& other);
    AnimatedGeometryPrimitive& operator=(AnimatedGeometryPrimitive&& other);

    // Animation player is set from AnimationPlayer's `add` method.
    AnimationPlayer* get_animation_player() const;

    const SkeletonPose& get_skeleton_pose() const;
    SkeletonPose& get_skeleton_pose();

    const SharedPtr<Animation>& get_animation() const;
    void set_animation(SharedPtr<Animation> animation);

    // The joint matrices are retrieved from skeleton pose.
    Vector<float4x4> get_model_space_joint_matrices(MemoryResource& memory_resource) override;

    float get_animation_time() const;
    void set_animation_time(float value);

    float get_animation_speed() const;
    void set_animation_speed(float value);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void geometry_loaded() override;

private:
    AnimationPlayer* m_animation_player;
    SkeletonPose m_skeleton_pose;
    SharedPtr<Animation> m_animation;
    float m_animation_time;
    float m_animation_speed;

    // Friendship is needed to access `m_animation_player`.
    friend class AnimationPlayer;
};

} // namespace kw
