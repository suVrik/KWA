#pragma once

#include <core/containers/string.h>
#include <core/containers/unordered_map.h>
#include <core/containers/vector.h>
#include <core/math/float4x4.h>

namespace kw {

class Skeleton {
public:
    Skeleton();
    Skeleton(Vector<uint32_t>&& parent_joints, Vector<float4x4>&& inverse_bind_matrices,
             Vector<float4x4>&& bind_matrices, UnorderedMap<String, uint32_t>&& joint_mapping);

    size_t get_joint_count() const;

    uint32_t get_parent_joint(uint32_t joint_index) const;
    const float4x4& get_inverse_bind_matrix(uint32_t joint_index) const;
    const float4x4& get_bind_matrix(uint32_t joint_index) const;
    const String& get_joint_name(uint32_t joint_index) const;

    // Returns UINT32_MAX on failure.
    uint32_t get_joint_index(const String& name) const;

private:
    Vector<uint32_t> m_parent_joints;
    Vector<float4x4> m_inverse_bind_matrices;
    Vector<float4x4> m_bind_matrices;
    UnorderedMap<String, uint32_t> m_joint_mapping;
};

} // namespace kw
