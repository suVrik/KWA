#pragma once

#include "core/memory/noop_memory_resource.h"

#include <memory>

namespace kw {

template <typename T, typename Allocator>
class UniquePtrDeleter {
public:
    static_assert(!std::is_array_v<T>);

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

template <typename T>
class UniquePtrDeleter<T, MemoryResourceAllocator<T>> {
public:
    static_assert(!std::is_array_v<T>);

    UniquePtrDeleter()
        : allocator(NoopMemoryResource::instance())
    {
    }

    UniquePtrDeleter(const MemoryResourceAllocator<T>& allocator_)
        : allocator(allocator_)
    {
    }
    
    void operator()(T* ptr) {
        ptr->~T();

        allocator.deallocate(ptr, 1);
    }

    MemoryResourceAllocator<T> allocator;
};

template <typename T, typename Allocator = MemoryResourceAllocator<std::remove_extent_t<T>>>
using UniquePtr = std::unique_ptr<T, UniquePtrDeleter<T, Allocator>>;

template <typename T, class... Args>
UniquePtr<T> allocate_unique(MemoryResource& memory_resource, Args&&... args) {
    T* ptr = new (memory_resource.allocate<T>()) T(std::forward<Args>(args)...);
    return UniquePtr<T>(ptr, UniquePtrDeleter<T, MemoryResourceAllocator<T>>(memory_resource));
}

template<class T, class U>
std::enable_if_t<!std::is_array_v<T>, UniquePtr<T>> static_pointer_cast(UniquePtr<U>&& another) {
    return UniquePtr<T>(static_cast<T*>(another.release()), UniquePtrDeleter<T, MemoryResourceAllocator<T>>(another.get_deleter().allocator));
}

} // namespace kw
