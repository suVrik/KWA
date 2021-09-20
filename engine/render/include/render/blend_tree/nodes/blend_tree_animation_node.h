#pragma once

#include "render/blend_tree/nodes/blend_tree_node.h"

#include <core/containers/shared_ptr.h>

namespace kw {

class Animation;

class BlendTreeAnimationNode : public BlendTreeNode {
public:
    BlendTreeAnimationNode(SharedPtr<Animation>&& animation);

    SkeletonPose compute(const BlendTreeContext& context) const override;

private:
    SharedPtr<Animation> m_animation;
};

} // namespace kw
