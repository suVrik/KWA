#pragma once

#include "render/geometry/geometry_primitive.h"
#include "render/geometry/skeleton_pose.h"

namespace kw {

class AnimatedGeometryPrimitive : public GeometryPrimitive {
public:
    AnimatedGeometryPrimitive(MemoryResource& memory_resource,
                              SharedPtr<Geometry> geometry = nullptr,
                              SharedPtr<Material> material = nullptr,
                              const transform& local_transform = transform());

    const SkeletonPose& get_skeleton_pose() const;
    SkeletonPose& get_skeleton_pose();

    // The joint matrices are retrieved from skeleton pose.
    Vector<float4x4> get_model_space_joint_matrices(MemoryResource& memory_resource) override;

protected:
    void geometry_loaded() override;

private:
    SkeletonPose m_skeleton_pose;
};

} // namespace kw
