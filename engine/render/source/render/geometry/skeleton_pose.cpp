#include "render/geometry/skeleton_pose.h"
#include "render/geometry/skeleton.h"

namespace kw {

SkeletonPose::SkeletonPose(MemoryResource& memory_resource)
    : m_joint_space_matrices(memory_resource)
    , m_model_space_matrices(memory_resource)
{
}
    
const Vector<float4x4>& SkeletonPose::get_joint_space_matrices() const {
    return m_joint_space_matrices;
}

void SkeletonPose::set_joint_space_matrix(uint32_t joint_index, const float4x4& matrix) {
    m_joint_space_matrices.resize(std::max(m_joint_space_matrices.size(), joint_index + 1ull));
    m_joint_space_matrices[joint_index] = matrix;
}

const Vector<float4x4>& SkeletonPose::get_model_space_matrices() const {
    return m_model_space_matrices;
}

void SkeletonPose::build_model_space_matrices(const Skeleton& skeleton) {
    m_model_space_matrices.clear();
    m_model_space_matrices.insert(m_model_space_matrices.end(), m_joint_space_matrices.begin(), m_joint_space_matrices.end());

    for (uint32_t i = 0; i < m_model_space_matrices.size(); i++) {
        uint32_t parent_joint = skeleton.get_parent_joint(i);
        if (parent_joint != UINT32_MAX) {
            m_model_space_matrices[i] *= m_model_space_matrices[parent_joint];
        }
    }

    for (uint32_t i = 0; i < m_model_space_matrices.size(); i++) {
        m_model_space_matrices[i] = skeleton.get_inverse_bind_matrix(i) * m_model_space_matrices[i];
    }
}

} // namespace kw
