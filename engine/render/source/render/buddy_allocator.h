#include "render/render.h"

namespace kw {

class RenderBuddyAllocator {
public:
    static constexpr uint64_t INVALID_ALLOCATION = UINT64_MAX;

    RenderBuddyAllocator(MemoryResource& memory_resource, uint64_t root_size_log2, uint64_t leaf_size_log2);
    RenderBuddyAllocator(const RenderBuddyAllocator& other) = delete;
    RenderBuddyAllocator(RenderBuddyAllocator&& other);
    ~RenderBuddyAllocator();
    RenderBuddyAllocator& operator=(const RenderBuddyAllocator& other) = delete;
    RenderBuddyAllocator& operator=(RenderBuddyAllocator&& other) = delete;

    uint64_t allocate(uint64_t size, uint64_t alignment);
    void deallocate(uint64_t offset);

private:
    static constexpr uint32_t END = 0x07FFFFFF;
    static constexpr uint32_t BUSY = 0x07FFFFFE;

    struct Leaf {
        uint32_t next : 27;
        uint32_t depth : 5;
    };

    MemoryResource& m_memory_resource;

    uint64_t m_leaf_size_log2;
    uint64_t m_max_depth;

    uint32_t* m_heads;
    Leaf* m_leafs;
};

} // namespace kw
