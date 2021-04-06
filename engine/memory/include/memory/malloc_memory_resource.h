#pragma once

#include "memory/memory_resource.h"

namespace kw {

class MallocMemoryResource : public MemoryResource {
public:
    static MallocMemoryResource& instance();

    MallocMemoryResource(const MallocMemoryResource& original) = delete;
    MallocMemoryResource(MallocMemoryResource&& original) = delete;
    ~MallocMemoryResource() override = default;
    MallocMemoryResource& operator=(const MallocMemoryResource& original) = delete;
    MallocMemoryResource& operator=(MallocMemoryResource&& original) = delete;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

private:
    MallocMemoryResource() = default;
};

} // namespace kw
