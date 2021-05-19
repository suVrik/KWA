#pragma once

#include <memory/memory_resource.h>

#include <memory>

namespace kw {

template <typename T>
using SharedPtr = std::shared_ptr<T>;

template <typename T>
using WeakPtr = std::weak_ptr<T>;

template <typename T, typename... Args>
SharedPtr<T> allocate_shared(MemoryResource& memory_resource, Args&&... args) {
    return std::allocate_shared<T, MemoryResourceAllocator<T>, Args...>(MemoryResourceAllocator<T>(memory_resource), std::forward<Args>(args)...);
}

} // namespace kw
