#pragma once

#include <core/containers/vector.h>
#include <core/math/float4x4.h>

namespace kw {

class Skeleton;

class SkeletonPose {
public:
    explicit SkeletonPose(MemoryResource& memory_resource);
    
    const Vector<float4x4>& get_joint_space_matrices() const;

    void set_joint_space_matrix(uint32_t joint_index, const float4x4& matrix);

    const Vector<float4x4>& get_model_space_matrices() const;

    // Uses joint space matrices and skeleton hierarchy to build model space matrices.
    void build_model_space_matrices(const Skeleton& skeleton);

private:
    Vector<float4x4> m_joint_space_matrices;
    Vector<float4x4> m_model_space_matrices;

    friend class AnimatedGeometryPrimitive;
};

} // namespace kw
