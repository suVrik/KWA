#include "render/animation/animated_geometry_primitive.h"

#include <core/debug/assert.h>
#include <core/memory/malloc_memory_resource.h>

namespace kw {

AnimatedGeometryPrimitive::AnimatedGeometryPrimitive(MemoryResource& memory_resource, SharedPtr<Geometry> geometry,
                                                     SharedPtr<Material> material, const transform& local_transform)
    : GeometryPrimitive(geometry, material, local_transform)
    , m_skeleton_pose(memory_resource)
{
}

void AnimatedGeometryPrimitive::set_geometry(SharedPtr<Geometry> geometry) {
    GeometryPrimitive::set_geometry(geometry);

    // Geometry has changed, animation manager must reinitialize joints from bind pose.
    m_skeleton_pose.m_joint_space_matrices.clear();
}

const SkeletonPose& AnimatedGeometryPrimitive::get_skeleton_pose() const {
    return m_skeleton_pose;
}

SkeletonPose& AnimatedGeometryPrimitive::get_skeleton_pose() {
    return m_skeleton_pose;
}

Vector<float4x4> AnimatedGeometryPrimitive::get_model_space_joint_matrices(MemoryResource& memory_resource) {
    return Vector<float4x4>(m_skeleton_pose.get_model_space_matrices(), memory_resource);
}

} // namespace kw
