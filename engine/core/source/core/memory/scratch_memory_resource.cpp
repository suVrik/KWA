#include "core/memory/scratch_memory_resource.h"
#include "core/debug/assert.h"

namespace kw {

ScratchMemoryResource::ScratchMemoryResource(MemoryResource& memory_resource, size_t capacity)
    : m_memory_resource(&memory_resource)
{
    m_begin = memory_resource.allocate(capacity, 1);
    m_current = m_begin;
    m_end = static_cast<uint8_t*>(m_begin) + capacity;
}

ScratchMemoryResource::ScratchMemoryResource(void* data, size_t capacity)
    : m_memory_resource(nullptr)
    , m_begin(data)
    , m_current(data)
    , m_end(static_cast<uint8_t*>(data) + capacity)
{
}

ScratchMemoryResource::~ScratchMemoryResource() {
    if (m_memory_resource != nullptr) {
        m_memory_resource->deallocate(m_begin);
    }
}

void* ScratchMemoryResource::allocate(size_t size, size_t alignment) {
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

void* ScratchMemoryResource::reallocate(void* memory, size_t size, size_t alignment) {
    KW_ASSERT(memory == nullptr || (memory >= m_begin && memory < m_current), "Invalid reallocation.");

    void* result = allocate(size, alignment);
    if (memory != nullptr) {
        std::memcpy(result, memory, size);
        deallocate(memory);
    }
    return result;
}

void ScratchMemoryResource::deallocate(void* memory) {
    // No-op.
}

void ScratchMemoryResource::reset() {
    m_current = m_begin;
}

} // namespace kw
