#include "render/buddy_allocator.h"

#include <core/debug/assert.h>
#include <core/memory/memory_resource.h>

#include <cstddef>

namespace kw {

RenderBuddyAllocator::RenderBuddyAllocator(MemoryResource& memory_resource, uint64_t root_size_log2, uint64_t leaf_size_log2)
    : m_memory_resource(memory_resource)
    , m_leaf_size_log2(leaf_size_log2)
    , m_max_depth(root_size_log2 - leaf_size_log2)
{
    KW_ASSERT(root_size_log2 >= leaf_size_log2, "Root size must be not less than leaf size.");
    KW_ASSERT(root_size_log2 - leaf_size_log2 < 27, "Binary tree height must be less than 27.");
    KW_ASSERT(leaf_size_log2 > 0, "Leaf size must be greater than 0.");

    m_heads = memory_resource.allocate<uint32_t>(m_max_depth + 1ull);
    m_leafs = memory_resource.allocate<Leaf>(1ull << m_max_depth);

    for (uint32_t i = 0; i < m_max_depth; i++) {
        m_heads[i] = END;
    }

    m_heads[m_max_depth] = 0;

    m_leafs[0].depth = m_max_depth;
    m_leafs[0].next = END;
}

RenderBuddyAllocator::RenderBuddyAllocator(RenderBuddyAllocator&& other)
    : m_memory_resource(other.m_memory_resource)
    , m_leaf_size_log2(other.m_leaf_size_log2)
    , m_max_depth(other.m_max_depth)
    , m_heads(other.m_heads)
    , m_leafs(other.m_leafs)
{
    other.m_heads = nullptr;
    other.m_leafs = nullptr;
}

RenderBuddyAllocator::~RenderBuddyAllocator() {
    KW_ASSERT(m_heads == nullptr || m_heads[m_max_depth] != END, "Not all memory is deallocated.");

    m_memory_resource.deallocate(m_leafs);
    m_memory_resource.deallocate(m_heads);
}

uint64_t RenderBuddyAllocator::allocate(uint64_t size, uint64_t alignment) {
    KW_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be power of two.");
    KW_ASSERT(size > 0, "Size must be greater than zero.");

    uint32_t depth = 0;
    uint64_t allocation_size = 1ull << m_leaf_size_log2;

    // Search for the smallest available node.
    while (depth < m_max_depth && (allocation_size < size || m_heads[depth] == END)) {
        depth++;
        allocation_size <<= 1;
    }

    // Check whether any node is found.
    if (depth == m_max_depth && (allocation_size < size || m_heads[m_max_depth] == END)) {
        return INVALID_ALLOCATION;
    }

    // Split this node into two smaller nodes recursively.
    uint32_t local_offset = m_heads[depth];
    KW_ASSERT(m_leafs[local_offset].next != BUSY);
    KW_ASSERT(m_leafs[local_offset].depth == depth);

    // Remove this node from this depth's linked list.
    m_heads[depth] = m_leafs[local_offset].next;

    // Split as many nodes as we can.
    while (depth > 0 && (allocation_size >> 1) >= size) {
        uint32_t buddy_offset = local_offset ^ (1u << (depth - 1));
        // Buddy has undefined next and depth.

        depth--;
        allocation_size >>= 1;

        // Insert buddy node into the next depth's linked list.
        uint32_t old_head = m_heads[depth];
        m_heads[depth] = buddy_offset;
        m_leafs[buddy_offset].next = old_head;
        m_leafs[buddy_offset].depth = depth;
    }

    // Mark the node as busy, so it's buddy node doesn't merge into a parent node. Depth could've changed too.
    m_leafs[local_offset].next = BUSY;
    m_leafs[local_offset].depth = depth;

    // Return absolute offset.
    return static_cast<uint64_t>(local_offset) << m_leaf_size_log2;
}

void RenderBuddyAllocator::deallocate(uint64_t offset) {
    if (offset != INVALID_ALLOCATION) {
        KW_ASSERT(((offset >> m_leaf_size_log2) << m_leaf_size_log2) == offset);

        uint32_t local_offset = static_cast<uint32_t>(offset >> m_leaf_size_log2);
        KW_ASSERT(m_leafs[local_offset].next == BUSY);

        uint32_t depth = m_leafs[local_offset].depth;
        KW_ASSERT(depth <= m_max_depth);

        uint32_t buddy_offset = local_offset ^ (1u << depth);

        // Merge as many nodes as we can.
        while (depth < m_max_depth && (m_leafs[buddy_offset].next != BUSY && m_leafs[buddy_offset].depth == depth)) {
            // Buddy offset must be stored in this depth's linked list.
            uint32_t current_offset = m_heads[depth];
            KW_ASSERT(current_offset != END);

            // Remove buddy offset from this depth's linked list.
            if (current_offset == buddy_offset) {
                // Buddy offset is stored in the head.
                m_heads[depth] = m_leafs[buddy_offset].next;
            } else {
                // Buddy offset is stored somewhere inside linked list.
                while (m_leafs[current_offset].next != buddy_offset) {
                    current_offset = m_leafs[current_offset].next;
                    KW_ASSERT(current_offset != END);
                }
                m_leafs[current_offset].next = m_leafs[buddy_offset].next;
            }

            depth++;

            // Calculate parent offset.
            local_offset = local_offset & buddy_offset;

            // Calculate parent buddy's offset.
            buddy_offset = local_offset ^ (1u << depth);
        }

        // Depth could've changed.
        m_leafs[local_offset].depth = depth;

        // Store offset in a linked list for other allocations.
        uint32_t old_head = m_heads[depth];
        m_heads[depth] = local_offset;
        m_leafs[local_offset].next = old_head;
    }
}

} // namespace kw
