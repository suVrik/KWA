#pragma once

#include <core/containers/unordered_map.h>
#include <core/containers/string.h>

namespace kw {

class SkeletonPose;

struct BlendTreeContext {
    const UnorderedMap<String, float>& attributes;
    MemoryResource& transient_memory_resource;
    float timestamp;
};

class BlendTreeNode {
public:
    virtual ~BlendTreeNode() = default;

    // Compute skeleton pose from the given attributes.
    virtual SkeletonPose compute(const BlendTreeContext& context) const = 0;
};

} // namespace kw
