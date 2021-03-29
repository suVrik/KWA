#include "memory/allocator_linear.h"

#include <debug/assert.h>

namespace kw {

MemoryResourceLinear::ResetPoint::ResetPoint(MemoryResourceLinear* memory_resource)
    : m_memory_resource(memory_resource)
    , m_current(memory_resource->m_current) {
}

MemoryResourceLinear::ResetPoint::~ResetPoint() {
    KW_ASSERT(m_memory_resource->m_current >= m_current);
    m_memory_resource->m_current = m_current;
}

MemoryResourceLinear::MemoryResourceLinear(size_t capacity)
    : m_begin(std::make_unique<char[]>(capacity))
    , m_current(m_begin.get())
    , m_end(m_current + capacity) {
}

void* MemoryResourceLinear::allocate(size_t size, size_t alignment) {
    KW_ASSERT((alignment & (alignment - 1)) == 0, "Alignment must be power of two.");
    
    char* old_value = m_current.load(std::memory_order_relaxed);
    while (true) {
        char* result = reinterpret_cast<char*>((reinterpret_cast<uintptr_t>(old_value) + (alignment - 1)) & ~(alignment - 1));
        if (m_current.compare_exchange_weak(old_value, result + size, std::memory_order_release, std::memory_order_relaxed)) {
            KW_ASSERT(result + size <= m_end, "Linear allocator overflow. Consider increasing capacity.");
            return result;
        }
    }
}

void MemoryResourceLinear::deallocate(void* memory) {
    KW_ASSERT(memory >= m_begin.get() && memory < m_current, "Invalid deallocation.");
}

MemoryResourceLinear::ResetPoint MemoryResourceLinear::reset() {
    return ResetPoint(this);
}

} // namespace kw
