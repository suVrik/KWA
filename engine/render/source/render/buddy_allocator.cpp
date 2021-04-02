#include "render/buddy_allocator.h"

#include <debug/assert.h>

namespace kw {

BuddyAllocator::BuddyAllocator(size_t largest_allocation_pow2, size_t smallest_allocation_pow2)
    : m_min_size(1 << smallest_allocation_pow2)
    , m_max_depth(largest_allocation_pow2 - smallest_allocation_pow2)
    , m_heads(std::make_unique<uint16_t[]>(m_max_depth + 1))
    , m_data(std::make_unique<uint16_t[]>(1 << m_max_depth))
{
    // Only the largest node is available.
    for (size_t i = 0; i < m_max_depth; i++) {
        m_heads[i] = UINT16_MAX;
    }
    m_heads[m_max_depth] = 0;
    m_data[0] = UINT16_MAX;
}

size_t BuddyAllocator::allocate(size_t size) {
    size_t depth = 0;
    size_t allocation_size = m_min_size;

    // Search for the smallest available node.
    while (depth < m_max_depth && (allocation_size < size || m_heads[depth] == UINT16_MAX)) {
        depth++;
        allocation_size *= 2;
    }

    // Check whether any node is found.
    if (depth == m_max_depth && (allocation_size < size || m_heads[m_max_depth] == UINT16_MAX)) {
        return INVALID_ALLOCATION;
    }

    // Split as many nodes as we can.
    while (depth > 0 && allocation_size / 2 >= size) {
        // Split this node into two smaller nodes.
        uint16_t local_offset = m_heads[depth];
        uint16_t buddy_offset = local_offset ^ (1 << (depth - 1));

        // Remove this node from this depth's linked list.
        m_heads[depth] = m_data[local_offset];

        depth--;
        allocation_size /= 2;

        // Insert these two smaller nodes into the next depth's linked list.
        // Head points to local, local points to buddy, buddy points to old head.
        uint16_t old_head = m_heads[depth];
        m_heads[depth] = local_offset;
        m_data[local_offset] = buddy_offset;
        m_data[buddy_offset] = old_head;
    }

    uint16_t local_offset = m_heads[depth];

    // Remove this node from this depth's linked list.
    m_heads[depth] = m_data[local_offset];

    // Store depth in this offset for deallocate.
    m_data[local_offset] = depth | BUSY_BIT;

    return local_offset * m_min_size;
}

void BuddyAllocator::deallocate(size_t offset) {
    KW_ASSERT((offset & (m_min_size - 1)) == 0);
    KW_ASSERT(offset < (m_min_size << m_max_depth));

    uint16_t local_offset = offset / m_min_size;
    KW_ASSERT((m_data[local_offset] & BUSY_BIT) == BUSY_BIT);

    uint16_t depth = m_data[local_offset] & (~BUSY_BIT);
    KW_ASSERT(depth <= m_max_depth);

    uint16_t buddy_offset = local_offset ^ (1 << depth);

    // Merge as many nodes as we can.
    while (depth < m_max_depth && ((m_data[buddy_offset] & BUSY_BIT) == 0 || m_data[buddy_offset] == UINT16_MAX)) {
        // Remove buddy offset from this depth's linked list.
        uint16_t current_offset = m_heads[depth];
        if (current_offset == buddy_offset) {
            // Buddy offset is stored in the head.
            m_heads[depth] = m_data[buddy_offset];
        } else {
            // Buddy offset is stored somewhere inside linked list.
            while (m_data[current_offset] != buddy_offset) {
                current_offset = m_data[current_offset];
            }
            m_data[current_offset] = m_data[buddy_offset];
        }

        depth++;

        // Calculate parent offset.
        local_offset = local_offset & buddy_offset;

        // Calculate parent buddy's offset.
        buddy_offset = local_offset ^ (1 << depth);
    }

    // Store offset in a linked list for other allocations.
    uint16_t old_head = m_heads[depth];
    m_heads[depth] = local_offset;
    m_data[local_offset] = old_head;
}

bool BuddyAllocator::is_empty() const {
    return m_heads[m_max_depth] == 0;
}

} // namespace kw
