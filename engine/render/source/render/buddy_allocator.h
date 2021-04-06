#include "render/render.h"

namespace kw {

class RenderBuddyAllocator {
public:
    static constexpr size_t INVALID_ALLOCATION = SIZE_MAX;

    RenderBuddyAllocator(MemoryResource& memory_resource, size_t root_size_log2, size_t leaf_size_log2);
    RenderBuddyAllocator(const RenderBuddyAllocator& other) = delete;
    RenderBuddyAllocator(RenderBuddyAllocator&& other);
    ~RenderBuddyAllocator();
    RenderBuddyAllocator& operator=(const RenderBuddyAllocator& other) = delete;
    RenderBuddyAllocator& operator=(RenderBuddyAllocator&& other) = delete;

    size_t allocate(size_t size, size_t alignment);
    void deallocate(size_t offset);

private:
    static constexpr uint32_t END = 0x07FFFFFF;
    static constexpr uint32_t BUSY = 0x07FFFFFE;

    struct Leaf {
        uint32_t next : 27;
        uint32_t depth : 5;
    };

    MemoryResource& m_memory_resource;

    uint32_t m_leaf_size_log2;
    uint32_t m_max_depth;

    uint32_t* m_heads;
    Leaf* m_leafs;
};

} // namespace kw
