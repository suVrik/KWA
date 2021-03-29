#include "memory/memory_profiler.h"
#include "memory/allocator_linear.h"

#include <mutex>
#include <unordered_map>

namespace kw {

class MemoryProfiler::PImpl {
public:
    void allocate(const void* memory, size_t size, AllocationSubsystem subsystem, const char* name) {
        std::lock_guard<std::mutex> lock(m_allocations_mutex);

        Allocation allocation;
        allocation.size = size;
        allocation.subsystem = subsystem;
        allocation.name = name;

        m_allocations.emplace(memory, allocation);
    }

    void deallocate(const void* memory) {
        std::lock_guard<std::mutex> lock(m_allocations_mutex);

        m_allocations.erase(memory);
    }

    MemoryDump dump(MemoryResourceLinear* memory_resource) {
        std::lock_guard<std::mutex> lock(m_allocations_mutex);

        size_t allocation_count = m_allocations.size();
        Allocation* allocations = static_cast<Allocation*>(memory_resource->allocate(allocation_count * sizeof(Allocation), alignof(Allocation)));
        size_t allocation_index = 0;

        for (const auto& [memory, allocation] : m_allocations) {
            allocations[allocation_index++] = allocation;
        }

        return MemoryDump{ allocations, allocation_count };
    }

private:
    std::unordered_map<const void*, MemoryProfiler::Allocation> m_allocations;
    std::mutex m_allocations_mutex;
};

MemoryProfiler& MemoryProfiler::instance() {
    static MemoryProfiler memory_profiler;
    return memory_profiler;
}

MemoryProfiler::MemoryProfiler()
    : m_impl(new PImpl()) {
}

MemoryProfiler::~MemoryProfiler() = default;

void MemoryProfiler::allocate(const void* memory, size_t size, AllocationSubsystem subsystem, const char* name) {
    m_impl->allocate(memory, size, subsystem, name);
}

void MemoryProfiler::deallocate(const void* memory) {
    m_impl->deallocate(memory);
}

MemoryProfiler::MemoryDump MemoryProfiler::dump(MemoryResourceLinear* memory_resource) {
    return m_impl->dump(memory_resource);
}

} // namespace kw
