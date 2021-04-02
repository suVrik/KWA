#include "render/render.h"

#include <memory>

namespace kw {

class BuddyAllocator {
public:
    /** Returned by allocate when couldn't find suitable block. */
    static constexpr size_t INVALID_ALLOCATION = SIZE_MAX;

    /** For example largest_node_pow2 = 28, smallest_node_pow2 = 13 means the buddy allocator manages 256mb
        and the smallest possible allocation is 8kb. Allocator overhead for such case is only 64kb. */
    BuddyAllocator(size_t largest_node_pow2, size_t smallest_node_pow2);

    size_t allocate(size_t size);
    void deallocate(size_t offset);

    /** Check whether all allocator memory is available. */
    bool is_empty() const;

private:
    static constexpr uint16_t BUSY_BIT = 1 << 15;

    size_t m_min_size;
    size_t m_max_depth;

    std::unique_ptr<uint16_t[]> m_heads;
    std::unique_ptr<uint16_t[]> m_data;
};

} // namespace kw
