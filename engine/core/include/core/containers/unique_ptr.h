#pragma once

#include "core/memory/malloc_memory_resource.h"

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
        static_assert(!std::is_array_v<T>);

        ptr->~T();

        // Any other allocator may use `count`, so we can't do the trick we do for `MemoryResourceAllocator`
        // with trivially destructible types.
        allocator.deallocate(ptr, 1);
    }

    Allocator allocator;
};

template <typename T, typename U>
class UniquePtrDeleter<T, MemoryResourceAllocator<U>> {
public:
    // When T is `float[]`, Allocator is `MemoryResourceAllocator<float>`.
    static_assert(std::is_same_v<std::remove_extent_t<T>, U>);

    UniquePtrDeleter()
        : allocator(MallocMemoryResource::instance())
    {
    }

    UniquePtrDeleter(const MemoryResourceAllocator<U>& allocator_)
        : allocator(allocator_)
    {
    }

    UniquePtrDeleter(MemoryResource& memory_resource)
        : allocator(memory_resource)
    {
    }
    
    void operator()(U* ptr) {
        static_assert(!std::is_array_v<T> || std::is_trivially_destructible_v<std::remove_extent_t<T>>);

        // Array guarantees to store trivially destructible types, so no need to call destructor.
        if constexpr (!std::is_array_v<T>) {
            ptr->~T();
        }

        // `MemoryResourceAllocator` doesn't use count in any way.
        allocator.deallocate(ptr, 0);
    }

    MemoryResourceAllocator<U> allocator;
};

template <typename T, typename Allocator = MemoryResourceAllocator<std::remove_extent_t<T>>>
using UniquePtr = std::unique_ptr<T, UniquePtrDeleter<T, Allocator>>;

template <typename T, class... Args>
std::enable_if_t<!std::is_array_v<T>, UniquePtr<T>> allocate_unique(MemoryResource& memory_resource, Args&&... args) {
    return UniquePtr<T>(memory_resource.construct<T>(std::forward<Args>(args)...), memory_resource);
}

template <typename T, class... Args>
std::enable_if_t<std::is_array_v<T>, UniquePtr<T>> allocate_unique(MemoryResource& memory_resource, size_t count) {
    using U = std::remove_extent_t<T>;

    static_assert(std::is_default_constructible_v<U> && std::is_trivially_destructible_v<U>);

    U* items = memory_resource.allocate<U>(count);
    for (size_t i = 0; i < count; i++) {
        new (&items[i]) U();
    }
    return UniquePtr<T>(items, memory_resource);
}

template<class T, class U>
std::enable_if_t<!std::is_array_v<T>, UniquePtr<T>> static_pointer_cast(UniquePtr<U>&& another) {
    return UniquePtr<T>(static_cast<T*>(another.release()), UniquePtrDeleter<T, MemoryResourceAllocator<T>>(another.get_deleter().allocator));
}

} // namespace kw
