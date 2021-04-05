#pragma once

#include <cstdint>

namespace kw {

class MemoryResource {
public:
    virtual ~MemoryResource() = default;

    virtual void* allocate(size_t size, size_t alignment) = 0;
    virtual void* reallocate(void* memory, size_t size, size_t alignment) = 0;
    virtual void deallocate(void* memory) = 0;

    template <typename T>
    T* allocate(size_t count) {
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }
};

template <typename T>
class MemoryResourceAllocator {
public:
    using value_type = T;

    MemoryResourceAllocator(MemoryResource& memory_resource_)
        : memory_resource(memory_resource_) {
    }

    template <typename U>
    MemoryResourceAllocator(const MemoryResourceAllocator<U>& allocator)
        : memory_resource(allocator.memory_resource) {
    }

    T* allocate(size_t count) {
        return memory_resource.allocate<T>(count);
    }

    void deallocate(T* memory, size_t count) {
        memory_resource.deallocate(memory);
    }

    bool operator==(const MemoryResourceAllocator& other) const {
        return memory_resource == other.memory_resource;
    }

    bool operator!=(const MemoryResourceAllocator& other) const {
        return memory_resource != other.memory_resource;
    }

    MemoryResource& memory_resource;
};

} // namespace kw
