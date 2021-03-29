#pragma once

#include <atomic>
#include <memory>

namespace kw {

/** A huge continuous chunk of memory. Each allocation takes its next available piece. Deallocation is a no-op. */
class MemoryResourceLinear {
public:
    class ResetPoint {
    public:
        ResetPoint(MemoryResourceLinear* memory_resource);
        ~ResetPoint();

    private:
        MemoryResourceLinear* m_memory_resource;
        char* m_current;
    };

    MemoryResourceLinear(size_t capacity);

    void* allocate(size_t size, size_t alignment);
    void deallocate(void* memory);

    /** In the end of ResetPoint's lifetime, all further allocated memory will be freed. Must not be used in parallel code. */
    ResetPoint reset();

private:
    std::unique_ptr<char[]> m_begin;
    std::atomic<char*> m_current;
    char* m_end;
};

template <typename T>
class AllocatorLinear {
public:
    using value_type = T;

    AllocatorLinear(MemoryResourceLinear* memory_resource_)
        : memory_resource(memory_resource_) {
    }

    template <typename U>
    AllocatorLinear(const AllocatorLinear<U>& allocator)
        : memory_resource(allocator.memory_resource) {
    }

    T* allocate(size_t count) {
        return static_cast<T*>(memory_resource->allocate(sizeof(T) * count, alignof(T)));
    }

    void deallocate(T* memory, size_t count) {
        memory_resource->deallocate(memory);
    }

    bool operator==(const AllocatorLinear& other) const {
        return memory_resource == other.memory_resource;
    }

    bool operator!=(const AllocatorLinear& other) const {
        return memory_resource != other.memory_resource;
    }

    MemoryResourceLinear* const memory_resource;
};

} // namespace kw

#define KW_MEMORY_RESOURCE_RESET(memory_resource) auto reset_point = (memory_resource)->reset()
