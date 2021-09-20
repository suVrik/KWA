#pragma once

#include "render/animation/animated_geometry_primitive.h"

#include <core/containers/string.h>
#include <core/containers/unordered_map.h>

namespace kw {

class MotionGraph;
class ObjectNode;
class PrimitiveReflection;

class MotionGeometryPrimitive : public AnimatedGeometryPrimitive {
public:
    static UniquePtr<Primitive> create_from_markdown(PrimitiveReflection& reflection, const ObjectNode& node);

    MotionGeometryPrimitive(MemoryResource& memory_resource,
                            SharedPtr<MotionGraph> motion_graph = nullptr,
                            SharedPtr<Geometry> geometry = nullptr,
                            SharedPtr<Material> material = nullptr,
                            SharedPtr<Material> shadow_material = nullptr,
                            const transform& local_transform = transform());
    MotionGeometryPrimitive(const MotionGeometryPrimitive& other);
    MotionGeometryPrimitive(MotionGeometryPrimitive&& other) noexcept;
    ~MotionGeometryPrimitive() override;
    MotionGeometryPrimitive& operator=(const MotionGeometryPrimitive& other);
    MotionGeometryPrimitive& operator=(MotionGeometryPrimitive&& other) noexcept;

    const SharedPtr<MotionGraph>& get_motion_graph() const;
    void set_motion_graph(SharedPtr<MotionGraph> motion_graph);

    void emit_event(const String& name);

    uint32_t get_motion_index() const;
    float get_motion_time() const;

    // Remember the current pose and fade into new one in given time.
    void frozen_fade(float duration);

    float get_attribute(const String& name) const;
    void set_attribute(const String& name, float value);

    // Model space matrices before IK and multiplication by inverse bind matrices.
    const Vector<float4x4>& get_model_space_joint_pre_ik_matrices() const;

    // Fourth component goes for IK factor. IK factor of zero removes IK target.
    const float4& get_ik_target(uint32_t joint_a, uint32_t joint_b, uint32_t joint_c) const;
    void set_ik_target(uint32_t joint_a, uint32_t joint_b, uint32_t joint_c, const float4& target);

    UniquePtr<Primitive> clone(MemoryResource& memory_resource) const override;

protected:
    void update_animation(MemoryResource& transient_memory_resource, float elapsed_time);
    void geometry_loaded() override;

private:
    struct IkTarget {
        uint32_t joint_a;
        uint32_t joint_b;
        uint32_t joint_c;
        float4 target;
    };

    SharedPtr<MotionGraph> m_motion_graph;
    UnorderedMap<String, float> m_attributes;
    SkeletonPose m_pre_ik_skeleton_pose;
    Vector<IkTarget> m_ik_targets;
    SkeletonPose m_previous_skeleton_pose;
    uint32_t m_motion_index;
    float m_motion_time;
    float m_transition_time;
    float m_transition_duration;
};

} // namespace kw
