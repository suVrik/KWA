#pragma once

#include "memory/memory_resource.h"

namespace kw {

class PagedPoolMemoryResource final : public MemoryResource {
public:
    PagedPoolMemoryResource(MemoryResource& memory_resource, size_t allocation_size, size_t allocations_per_page);
    PagedPoolMemoryResource(const PagedPoolMemoryResource& original) = delete;
    PagedPoolMemoryResource(PagedPoolMemoryResource&& original) = delete;
    ~PagedPoolMemoryResource() override;
    PagedPoolMemoryResource& operator=(const PagedPoolMemoryResource& original) = delete;
    PagedPoolMemoryResource& operator=(PagedPoolMemoryResource&& original) = delete;

    void* allocate(size_t size, size_t alignment) override;
    void* reallocate(void* memory, size_t size, size_t alignment) override;
    void deallocate(void* memory) override;

private:
    void allocate_new_page(void* previous_page);

    MemoryResource& m_memory_resource;

    size_t m_allocation_size;
    size_t m_allocations_per_page;

    void* m_page_head;
    void* m_data_head;
};

} // namespace kw
