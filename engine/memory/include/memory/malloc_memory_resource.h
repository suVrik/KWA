#pragma once

#include "memory/memory_resource.h"

namespace kw {

class MallocMemoryResource : public MemoryResource {
public:
    MallocMemoryResource& instance();

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

private:
    MallocMemoryResource() = default;
};

} // namespace kw
