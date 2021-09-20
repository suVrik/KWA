#pragma once

#include "render/blend_tree/nodes/blend_tree_node.h"

#include <core/containers/map.h>
#include <core/containers/unique_ptr.h>

namespace kw {

class BlendTreeLerpNode : public BlendTreeNode {
public:
    BlendTreeLerpNode(String&& attribute, Map<float, UniquePtr<BlendTreeNode>>&& children);

    SkeletonPose compute(const BlendTreeContext& context) const override;

private:
    // TODO: Prefer `Name` to `String`.
    String m_attribute;

    // TODO: Prefer `Vector` to `Map`. A very few elements are expected.
    Map<float, UniquePtr<BlendTreeNode>> m_children;
};

} // namespace kw
