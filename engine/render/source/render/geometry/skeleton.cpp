#include "render/geometry/skeleton.h"

#include <core/debug/assert.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

// `MallocMemoryResource` is required here because of MSVC's STL debug iterators.
// It allocates some stuff in constructor (only in debug though).
Skeleton::Skeleton()
    : m_parent_joints(MallocMemoryResource::instance())
    , m_inverse_bind_matrices(MallocMemoryResource::instance())
    , m_bind_matrices(MallocMemoryResource::instance())
    , m_joint_mapping(MallocMemoryResource::instance())
{
}

Skeleton::Skeleton(Vector<uint32_t>&& parent_joints, Vector<float4x4>&& inverse_bind_matrices,
                   Vector<float4x4>&& bind_matrices, UnorderedMap<String, uint32_t>&& joint_mapping)
    : m_parent_joints(std::move(parent_joints))
    , m_inverse_bind_matrices(std::move(inverse_bind_matrices))
    , m_bind_matrices(std::move(bind_matrices))
    , m_joint_mapping(std::move(joint_mapping))
{
    KW_ASSERT(m_parent_joints.size() == m_inverse_bind_matrices.size(), "Mismatching skeleton data.");
    KW_ASSERT(m_parent_joints.size() == m_bind_matrices.size(), "Mismatching skeleton data.");
    KW_ASSERT(m_parent_joints.size() == m_joint_mapping.size(), "Mismatching skeleton data.");
}

size_t Skeleton::get_joint_count() const {
    return m_bind_matrices.size();
}

uint32_t Skeleton::get_parent_joint(uint32_t joint_index) const {
    KW_ASSERT(joint_index < get_joint_count(), "Invalid joint index.");

    return m_parent_joints[joint_index];
}

const float4x4& Skeleton::get_inverse_bind_matrix(uint32_t joint_index) const {
    KW_ASSERT(joint_index < get_joint_count(), "Invalid joint index.");

    return m_inverse_bind_matrices[joint_index];
}

const float4x4& Skeleton::get_bind_matrix(uint32_t joint_index) const {
    KW_ASSERT(joint_index < get_joint_count(), "Invalid joint index.");

    return m_bind_matrices[joint_index];
}

const String& Skeleton::get_joint_name(uint32_t joint_index) const {
    for (const auto& [name, index] : m_joint_mapping) {
        if (joint_index == index) {
            return name;
        }
    }

    static const String EMPTY_STRING(MallocMemoryResource::instance());
    return EMPTY_STRING;
}

uint32_t Skeleton::get_joint_index(const String& name) const {
    auto it = m_joint_mapping.find(name);
    if (it != m_joint_mapping.end()) {
        return it->second;
    }
    return UINT32_MAX;
}

} // namespace kw
