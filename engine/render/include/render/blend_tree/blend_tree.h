#pragma once

#include <core/containers/unique_ptr.h>

namespace kw {

class BlendTreeNode;

class BlendTree {
public:
    BlendTree() = default;
    explicit BlendTree(UniquePtr<BlendTreeNode>&& root);
    BlendTree(BlendTree&& other) noexcept;
    ~BlendTree();
    BlendTree& operator=(BlendTree&& other) noexcept;

    const BlendTreeNode* get_root_node() const;

    bool is_loaded() const;

private:
    UniquePtr<BlendTreeNode> m_root;
};

} // namespace kw
