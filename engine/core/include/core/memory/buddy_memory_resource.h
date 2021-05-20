#pragma once

#include "core/memory/memory_resource.h"

namespace kw {

class BuddyMemoryResource : public MemoryResource {
public:
    BuddyMemoryResource(MemoryResource& memory_resource, size_t root_size_log2, size_t leaf_size_log2);
    BuddyMemoryResource(const BuddyMemoryResource& other) = delete;
    BuddyMemoryResource(BuddyMemoryResource&& other) = delete;
    ~BuddyMemoryResource() override;
    BuddyMemoryResource& operator=(const BuddyMemoryResource& other) = delete;
    BuddyMemoryResource& operator=(BuddyMemoryResource&& other) = delete;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

    using MemoryResource::allocate;

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
    void* m_memory;
};

} // namespace kw
