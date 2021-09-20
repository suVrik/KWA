#include "render/geometry/skeleton_pose.h"
#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>

namespace kw {

SkeletonPose::SkeletonPose(MemoryResource& memory_resource)
    : m_joint_space_transforms(memory_resource)
    , m_model_space_matrices(memory_resource)
{
}

const Vector<transform>& SkeletonPose::get_joint_space_transforms() const {
    return m_joint_space_transforms;
}

void SkeletonPose::set_joint_space_transform(uint32_t joint_index, const transform& transform) {
    m_joint_space_transforms.resize(std::max(m_joint_space_transforms.size(), joint_index + 1ull));
    m_joint_space_transforms[joint_index] = transform;
}

const Vector<float4x4>& SkeletonPose::get_model_space_matrices() const {
    return m_model_space_matrices;
}

void SkeletonPose::build_model_space_matrices(const Skeleton& skeleton) {
    m_model_space_matrices.resize(m_joint_space_transforms.size());

    for (uint32_t i = 0; i < m_model_space_matrices.size(); i++) {
        m_model_space_matrices[i] = float4x4(m_joint_space_transforms[i]);
    }

    for (uint32_t i = 0; i < m_model_space_matrices.size(); i++) {
        uint32_t parent_joint = skeleton.get_parent_joint(i);
        if (parent_joint != UINT32_MAX) {
            m_model_space_matrices[i] *= m_model_space_matrices[parent_joint];
        }
    }
}

void SkeletonPose::apply_inverse_bind_matrices(const Skeleton& skeleton) {
    for (uint32_t i = 0; i < m_model_space_matrices.size(); i++) {
        m_model_space_matrices[i] = skeleton.get_inverse_bind_matrix(i) * m_model_space_matrices[i];
    }
}

void SkeletonPose::lerp(const SkeletonPose& other, float factor) {
    if (other.m_joint_space_transforms.empty() || equal(factor, 0.f)) {
        // Another skeleton pose is undefined, probably because animation that had to produce it is not loaded yet.
        return;
    }

    if (m_joint_space_transforms.empty() || equal(factor, 1.f)) {
        // This skeleton pose is undefined, probably because animation that had to produce it is not loaded yet.
        m_joint_space_transforms.assign(other.m_joint_space_transforms.begin(), other.m_joint_space_transforms.end());
        return;
    }

    KW_ASSERT(m_joint_space_transforms.size() == other.m_joint_space_transforms.size(), "Mismatching skeleton poses.");

    for (uint32_t i = 0; i < m_joint_space_transforms.size(); i++) {
        m_joint_space_transforms[i] = kw::lerp(m_joint_space_transforms[i], other.m_joint_space_transforms[i], factor);
    }
}

size_t SkeletonPose::get_joint_count() const {
    return m_joint_space_transforms.size();
}

} // namespace kw
