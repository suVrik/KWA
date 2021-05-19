#pragma once

#include <memory/memory_resource.h>

#include <memory>

namespace kw {

template <typename Allocator>
class AllocatorDeleter {
public:
    static_assert(!std::is_array_v<Allocator::value_type>, "Arrays are not yet supported by this deleter.");

    AllocatorDeleter(const Allocator& allocator_)
        : allocator(allocator_)
    {
    }

    void operator()(Allocator::value_type* ptr) const {
        allocator.deallocate(ptr, 1);
        ptr->~Allocator::value_type();
    }

    Allocator allocator;
};

template <typename T, typename Allocator = MemoryResourceAllocator<T>>
using UniquePtr = std::unique_ptr<T, AllocatorDeleter<Allocator>>;

template <typename T, class... Args>
UniquePtr<T> allocate_unique(MemoryResource& memory_resource, Args&&... args) {
    static_assert(!std::is_array_v<T>, "Arrays are not yet supported by UniquePtr.");

    return UniquePtr<T>(new (allocator.allocate(1)) T(std::forward<Args>(args)...), memory_resource);
}

} // namespace kw
