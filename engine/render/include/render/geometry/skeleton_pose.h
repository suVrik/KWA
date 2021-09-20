#pragma once

#include <core/containers/vector.h>
#include <core/math/float4x4.h>
#include <core/math/transform.h>

namespace kw {

class Skeleton;

class SkeletonPose {
public:
    explicit SkeletonPose(MemoryResource& memory_resource);
    
    const Vector<transform>& get_joint_space_transforms() const;

    void set_joint_space_transform(uint32_t joint_index, const transform& transform);

    const Vector<float4x4>& get_model_space_matrices() const;

    // Uses joint space matrices and skeleton hierarchy to build model space matrices.
    void build_model_space_matrices(const Skeleton& skeleton);

    // The number of joint space matrices.
    size_t get_joint_count() const;

private:
    Vector<transform> m_joint_space_transforms;
    Vector<float4x4> m_model_space_matrices;
};

} // namespace kw
