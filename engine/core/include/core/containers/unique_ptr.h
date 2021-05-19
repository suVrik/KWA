#pragma once

#include "core/memory/memory_resource.h"

#include <memory>

namespace kw {

template <typename T, typename Allocator>
class UniquePtrDeleter {
public:
    UniquePtrDeleter(const Allocator& allocator_)
        : allocator(allocator_)
    {
    }

    void operator()(T* ptr) {
        ptr->~T();

        allocator.deallocate(ptr, 1);
    }

    Allocator allocator;
};

template <typename T, typename Allocator>
class UniquePtrDeleter<T[], Allocator> {
public:
    UniquePtrDeleter(const Allocator& allocator_, size_t size_)
        : allocator(allocator_)
        , size(size_)
    {
    }

    void operator()(T* ptr) {
        for (size_t i = 0; i < size; i++) {
            ptr[i].~T();
        }

        allocator.deallocate(ptr, size);
    }

    Allocator allocator;
    size_t size;
};

template <typename T, typename Allocator = MemoryResourceAllocator<std::remove_extent_t<T>>>
using UniquePtr = std::unique_ptr<T, UniquePtrDeleter<T, Allocator>>;

template <typename T, class... Args, std::enable_if_t<!std::is_array_v<T>, int> = 0>
UniquePtr<T> allocate_unique(MemoryResource& memory_resource, Args&&... args) {
    T* ptr = new (memory_resource.allocate<T>(1)) T(std::forward<Args>(args)...);
    return UniquePtr<T>(ptr, UniquePtrDeleter<T, MemoryResourceAllocator<T>>(memory_resource));
}

template <typename T, class... Args, std::enable_if_t<std::is_array_v<T>, int> = 0>
UniquePtr<T> allocate_unique(MemoryResource& memory_resource, size_t size) {
    using U = std::remove_extent_t<T>;

    U* ptr = memory_resource.allocate<U>(size);
    for (size_t i = 0; i < size; i++) {
        new (ptr + i) U;
    }

    return UniquePtr<T>(ptr, UniquePtrDeleter<T, MemoryResourceAllocator<U>>(memory_resource, size));
}

} // namespace kw
