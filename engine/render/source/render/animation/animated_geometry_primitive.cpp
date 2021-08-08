#include "render/animation/animated_geometry_primitive.h"
#include "render/geometry/geometry.h"
#include "render/geometry/skeleton.h"

namespace kw {

AnimatedGeometryPrimitive::AnimatedGeometryPrimitive(MemoryResource& memory_resource, SharedPtr<Geometry> geometry,
                                                     SharedPtr<Material> material, const transform& local_transform)
    : GeometryPrimitive(geometry, material, local_transform)
    , m_skeleton_pose(memory_resource)
{
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

void AnimatedGeometryPrimitive::geometry_loaded() {
    GeometryPrimitive::geometry_loaded();

    const Skeleton* skeleton = get_geometry()->get_skeleton();
    if (skeleton != nullptr) {
        for (uint32_t i = 0; i < skeleton->get_joint_count(); i++) {
            m_skeleton_pose.set_joint_space_matrix(i, skeleton->get_bind_matrix(i));
        }
        m_skeleton_pose.build_model_space_matrices(*skeleton);
    }
}

} // namespace kw
