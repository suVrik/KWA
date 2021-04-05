#pragma once

#include "memory/memory_resource.h"

#include <atomic>

namespace kw {

class LinearMemoryResource : public MemoryResource {
public:
    LinearMemoryResource(MemoryResource& memory_resource, size_t capacity);
    ~LinearMemoryResource() override;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

    void reset();

private:
    MemoryResource& m_memory_resource;

    void* m_begin;
    std::atomic<void*> m_current;
    void* m_end;
};

} // namespace kw
