#pragma once

#include "render/geometry/geometry_primitive.h"
#include "render/geometry/skeleton_pose.h"

namespace kw {

class AnimationPlayer;

class AnimatedGeometryPrimitive : public GeometryPrimitive {
public:
    AnimatedGeometryPrimitive(MemoryResource& memory_resource,
                              SharedPtr<Geometry> geometry = nullptr,
                              SharedPtr<Material> material = nullptr,
                              SharedPtr<Material> shadow_material = nullptr,
                              const transform& local_transform = transform());
    AnimatedGeometryPrimitive(const AnimatedGeometryPrimitive& other);
    AnimatedGeometryPrimitive(AnimatedGeometryPrimitive&& other) noexcept;
    ~AnimatedGeometryPrimitive() override;
    AnimatedGeometryPrimitive& operator=(const AnimatedGeometryPrimitive& other);
    AnimatedGeometryPrimitive& operator=(AnimatedGeometryPrimitive&& other) noexcept;

    // Animation player is set from AnimationPlayer's `add` method.
    AnimationPlayer* get_animation_player() const;

    const SkeletonPose& get_skeleton_pose() const;
    SkeletonPose& get_skeleton_pose();

    // The joint matrices are retrieved from skeleton pose.
    Vector<float4x4> get_model_space_joint_matrices(MemoryResource& memory_resource) override;

    float get_animation_speed() const;
    void set_animation_speed(float value);

protected:
    virtual void update_animation(MemoryResource& transient_memory_resource, float elapsed_time) = 0;
    void geometry_loaded() override;

private:
    AnimationPlayer* m_animation_player;
    SkeletonPose m_skeleton_pose;
    float m_animation_speed;

    // Friendship is needed to access `m_animation_player` and `update_animation`.
    friend class AnimationPlayer;
};

} // namespace kw
