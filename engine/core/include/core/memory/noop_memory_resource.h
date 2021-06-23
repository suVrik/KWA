#pragma once

#include "core/memory/memory_resource.h"

namespace kw {

class NoopMemoryResource : public MemoryResource {
public:
    static NoopMemoryResource& instance();

    NoopMemoryResource(const NoopMemoryResource& original) = delete;
    NoopMemoryResource(NoopMemoryResource&& original) = delete;
    ~NoopMemoryResource() override = default;
    NoopMemoryResource& operator=(const NoopMemoryResource& original) = delete;
    NoopMemoryResource& operator=(NoopMemoryResource&& original) = delete;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

    using MemoryResource::allocate;

private:
    NoopMemoryResource() = default;
};

} // namespace kw
