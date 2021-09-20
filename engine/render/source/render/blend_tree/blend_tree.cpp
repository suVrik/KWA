#include "render/blend_tree/blend_tree.h"
#include "render/blend_tree/nodes/blend_tree_node.h"

namespace kw {

BlendTree::BlendTree(UniquePtr<BlendTreeNode>&& root)
    : m_root(std::move(root))
{
}

BlendTree::BlendTree(BlendTree&& other) noexcept = default;
BlendTree::~BlendTree() = default;
BlendTree& BlendTree::operator=(BlendTree&& other) noexcept = default;

const BlendTreeNode* BlendTree::get_root_node() const {
    return m_root.get();
}

bool BlendTree::is_loaded() const {
    return static_cast<bool>(m_root);
}

} // namespace kw
