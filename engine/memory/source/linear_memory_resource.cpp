#include "memory/linear_memory_resource.h"

#include <debug/assert.h>

namespace kw {

LinearMemoryResource::LinearMemoryResource(MemoryResource& memory_resource, size_t capacity)
    : m_memory_resource(memory_resource)
{
    m_begin = memory_resource.allocate(capacity, 1);
    m_current = m_begin;
    m_end = static_cast<std::byte*>(m_begin) + capacity;
}

LinearMemoryResource::~LinearMemoryResource() {
    m_memory_resource.deallocate(m_begin);
}

void* LinearMemoryResource::allocate(size_t size, size_t alignment) {
    KW_ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Alignment must be power of two.");
    KW_ASSERT(size > 0, "Size must be greater than zero.");
    
    void* old_value = m_current.load(std::memory_order_relaxed);
    while (true) {
        void* result = reinterpret_cast<void*>((reinterpret_cast<size_t>(old_value) + (alignment - 1)) & ~(alignment - 1));
        if (m_current.compare_exchange_weak(old_value, reinterpret_cast<void*>(reinterpret_cast<size_t>(result) + size), std::memory_order_release, std::memory_order_relaxed)) {
            KW_ASSERT(reinterpret_cast<void*>(reinterpret_cast<size_t>(result) + size) <= m_end, "Linear allocator overflow. Consider increasing capacity.");
            return result;
        }
    }
}

void* LinearMemoryResource::reallocate(void* memory, size_t size, size_t alignment) {
    KW_ASSERT(memory == nullptr || (memory >= m_begin && memory < m_current), "Invalid reallocation.");

    void* result = allocate(size, alignment);
    if (memory != nullptr) {
        std::memcpy(result, memory, size);
        deallocate(memory);
    }
    return result;
}

void LinearMemoryResource::deallocate(void* memory) {
    KW_ASSERT(memory == nullptr || (memory >= m_begin && memory < m_current), "Invalid deallocation.");
}

void LinearMemoryResource::reset() {
    m_current = m_begin;
}

} // namespace kw
