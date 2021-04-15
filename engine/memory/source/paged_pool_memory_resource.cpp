#include "memory/paged_pool_memory_resource.h"

#include <debug/assert.h>

#include <cstddef>
#include <cstring>

namespace kw {

PagedPoolMemoryResource::PagedPoolMemoryResource(MemoryResource& memory_resource, size_t allocation_size, size_t allocations_per_page)
    : m_memory_resource(memory_resource)
    , m_allocation_size(allocation_size)
    , m_allocations_per_page(allocations_per_page)
{
    KW_ASSERT(allocation_size >= sizeof(void*), "Allocation size must be not less than 8 bytes.");
    KW_ASSERT(allocations_per_page > 0, "At least one allocation per page is required.");

    allocate_new_page(nullptr);
}

PagedPoolMemoryResource::~PagedPoolMemoryResource() {
    void* next_page;
    do {
        std::memcpy(&next_page, m_page_head, sizeof(void*));
        m_memory_resource.deallocate(m_page_head);
        m_page_head = next_page;
    } while (next_page != nullptr);
}

void* PagedPoolMemoryResource::allocate(size_t size, size_t alignment) {
    KW_ASSERT(size <= m_allocation_size, "Invalid size.");
    KW_ASSERT(m_allocation_size >= alignment && m_allocation_size % alignment == 0, "Invalid alignment.");

    if (m_data_head == nullptr) {
        allocate_new_page(m_page_head);
        KW_ASSERT(m_data_head != nullptr);
    }

    // Remove storage from the linked list and return it.
    void* result = m_data_head;
    std::memcpy(m_data_head, m_data_head, sizeof(void*));
    return result;
}

void* PagedPoolMemoryResource::reallocate(void* memory, size_t size, size_t alignment) {
    KW_ASSERT(size <= m_allocation_size, "Invalid size.");
    KW_ASSERT(m_allocation_size >= alignment && m_allocation_size % alignment == 0, "Invalid alignment.");

    // Reallocate for a pool memory resource doesn't make any sense.
    return memory;
}

void PagedPoolMemoryResource::deallocate(void* memory) {
    if (memory != nullptr) {
#ifndef NDEBUG
        KW_ASSERT(reinterpret_cast<size_t>(memory) % m_allocation_size == 0, "Invalid alignment.");

        bool valid_range = false;

        // Check whether address is inside some of the pages.
        void* current_page = m_page_head;
        do {
            void* items_start = reinterpret_cast<void*>((reinterpret_cast<size_t>(m_page_head) + sizeof(void*) + (m_allocation_size - 1)) & ~(m_allocation_size - 1));
            void* items_end = static_cast<uint8_t*>(items_start) + m_allocation_size * m_allocations_per_page;
            if (memory >= items_start && memory < items_end) {
                valid_range = true;
                break;
            }
            std::memcpy(&current_page, m_page_head, sizeof(void*));
        } while (current_page != nullptr);

        KW_ASSERT(valid_range, "Memory out of range.");
#endif

        // Add this storage to the linked list.
        std::memcpy(memory, &m_data_head, sizeof(void*));
        m_data_head = memory;
    }
}

void PagedPoolMemoryResource::allocate_new_page(void* previous_page) {
    // Allocate enough memory for page, pointer to the next page and page alignment.
    m_page_head = static_cast<void*>(m_memory_resource.allocate(m_allocation_size * m_allocations_per_page + sizeof(void*) + (m_allocation_size - 1), alignof(void*)));

    // Store address of the previous page at the beginning.
    std::memcpy(m_page_head, &previous_page, sizeof(void*));

    // Offset by next page pointer and align by allocation size.
    void* current_item = m_data_head = reinterpret_cast<void*>((reinterpret_cast<size_t>(m_page_head) + sizeof(void*) + (m_allocation_size - 1)) & ~(m_allocation_size - 1));
    void* next_item = static_cast<uint8_t*>(current_item) + m_allocation_size;

    // Each item (prior the last one) points to the next item.
    for (size_t i = 0; i + 1 < m_allocations_per_page; i++) {
        std::memcpy(current_item, &next_item, sizeof(void*));

        current_item = next_item;
        next_item = static_cast<uint8_t*>(current_item) + m_allocation_size;
    }

    // The last item points to nothing. Additional page allocation would be required.
    std::memset(current_item, 0, sizeof(void*));
}

} // namespace kw
