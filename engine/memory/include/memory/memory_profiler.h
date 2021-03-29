#pragma once

#include <memory>

namespace kw {

class MemoryResourceLinear;

enum class AllocationSubsystem {
    RENDER,
    OTHER,
};

constexpr size_t ALLOCATION_SUBSYSTEM_COUNT = 2;

class MemoryProfiler {
public:
    struct Allocation {
        size_t size;
        AllocationSubsystem subsystem;
        const char* name;
    };

    struct MemoryDump {
        Allocation* allocations;
        size_t allocation_count;
    };

    static MemoryProfiler& instance();

    MemoryProfiler();
    ~MemoryProfiler();

    void allocate(const void* memory, size_t size, AllocationSubsystem subsystem = AllocationSubsystem::OTHER, const char* name = nullptr);
    void deallocate(const void* memory);

    /** Return all active allocations, storage is allocated via specified linear memory resource. */
    MemoryDump dump(MemoryResourceLinear* memory_resource);

private:
    class PImpl;
    std::unique_ptr<PImpl> m_impl;
};

} // namespace kw

#ifdef KW_MEMORY_PROFILER
#define KW_MEMORY_PROFILER_ALLOCATE(memory, size, ...) kw::MemoryProfiler::instance().allocate((memory), (size), ##__VA_ARGS__)
#define KW_MEMORY_PROFILER_DEALLOCATE(memory) kw::MemoryProfiler::instance().deallocate(memory)
#else
#define KW_MEMORY_PROFILER_ALLOCATE(memory, size, ...) ((void)0)
#define KW_MEMORY_PROFILER_DEALLOCATE(memory) ((void)0)
#endif
