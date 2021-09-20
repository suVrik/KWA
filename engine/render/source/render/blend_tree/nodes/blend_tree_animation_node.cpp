#include "render/blend_tree/nodes/blend_tree_animation_node.h"
#include "render/animation/animation.h"
#include "render/geometry/skeleton_pose.h"

namespace kw {

BlendTreeAnimationNode::BlendTreeAnimationNode(SharedPtr<Animation>&& animation)
    : m_animation(std::move(animation))
{
}

SkeletonPose BlendTreeAnimationNode::compute(const BlendTreeContext& context) const {
    SkeletonPose result(context.transient_memory_resource);

    if (m_animation && m_animation->is_loaded()) {
        for (uint32_t i = 0; i < m_animation->get_joint_count(); i++) {
            result.set_joint_space_transform(i, m_animation->get_joint_transform(i, context.timestamp));
        }
    }

    return result;
}

} // namespace kw
