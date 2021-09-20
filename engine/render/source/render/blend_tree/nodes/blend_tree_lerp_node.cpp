#include "render/blend_tree/nodes/blend_tree_lerp_node.h"
#include "render/geometry/skeleton_pose.h"

#include <core/debug/assert.h>

namespace kw {

BlendTreeLerpNode::BlendTreeLerpNode(String&& attribute, Map<float, UniquePtr<BlendTreeNode>>&& children)
    : m_attribute(std::move(attribute))
    , m_children(std::move(children))
{
    KW_ASSERT(!m_children.empty(), "Invalid blend tree. At least one child is required.");
}

SkeletonPose BlendTreeLerpNode::compute(const BlendTreeContext& context) const {
    float value = 0.f;

    auto it1 = context.attributes.find(m_attribute);
    if (it1 != context.attributes.end()) {
        value = it1->second;
    }

    SkeletonPose result(context.transient_memory_resource);

    auto it2 = m_children.lower_bound(value);
    if (it2 == m_children.end()) {
        result = m_children.rbegin()->second->compute(context);
    } else {
        if (it2 != m_children.begin() && !equal(it2->first, value)) {
            auto it3 = std::prev(it2);

            float factor = clamp((value - it3->first) / (it2->first - it3->first), 0.f, 1.f);
            KW_ASSERT(std::isfinite(factor), "Invalid blend tree. Children with the same key are illegal.");

            result = it3->second->compute(context);
            result.lerp(it2->second->compute(context), factor);
        } else {
            result = it2->second->compute(context);
        }
    }

    return result;
}

} // namespace kw
