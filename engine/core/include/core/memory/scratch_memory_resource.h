#pragma once

#include "core/memory/memory_resource.h"

#include <atomic>

namespace kw {

class ScratchMemoryResource : public MemoryResource {
public:
    ScratchMemoryResource(MemoryResource& memory_resource, size_t capacity);
    ScratchMemoryResource(void* data, size_t capacity);
    ScratchMemoryResource(const ScratchMemoryResource& other) = delete;
    ScratchMemoryResource(ScratchMemoryResource&& other) = delete;
    ~ScratchMemoryResource() override;
    ScratchMemoryResource& operator=(const ScratchMemoryResource& original) = delete;
    ScratchMemoryResource& operator=(ScratchMemoryResource&& original) = delete;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

    using MemoryResource::allocate;

    void reset();

private:
    MemoryResource* m_memory_resource;

    void* m_begin;
    std::atomic<void*> m_current;
    void* m_end;
};

} // namespace kw
