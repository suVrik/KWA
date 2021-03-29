#pragma once

#include "memory/memory_profiler.h"

#include <malloc.h>

namespace kw {

template <typename T>
class AllocatorNamed {
public:
    using value_type = T;

    AllocatorNamed(AllocationSubsystem subsystem_ = AllocationSubsystem::OTHER, const char* name_ = nullptr)
#ifdef KW_MEMORY_PROFILER
        : subsystem(subsystem_)
        , name(name_)
#endif
    {
    }

    template <typename U>
    AllocatorNamed(const AllocatorNamed<U>& allocator)
#ifdef KW_MEMORY_PROFILER
        : subsystem(allocator.subsystem)
        , name(allocator.name)
#endif
    {
    }

    T* allocate(size_t count) {
        T* result = static_cast<T*>(_aligned_malloc(sizeof(T) * count, alignof(T)));
        KW_MEMORY_PROFILER_ALLOCATE(result, sizeof(T) * count, m_subsystem, m_name);
        return result;
    }

    void deallocate(T* memory, size_t count) {
        KW_MEMORY_PROFILER_DEALLOCATE(memory);
        _aligned_free(memory);
    }

    bool operator==(const AllocatorNamed& other) const {
        return true;
    }

    bool operator!=(const AllocatorNamed& other) const {
        return false;
    }

#ifdef KW_MEMORY_PROFILER
    const AllocationSubsystem subsystem;
    const char* const name;
#endif
};

} // namespace kw
