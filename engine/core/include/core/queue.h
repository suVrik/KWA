#pragma once

#include <memory/memory_resource.h>

#include <queue>

namespace kw {

template <typename T, typename Container = std::deque<T, MemoryResourceAllocator<T>>>
using Queue = std::queue<T, Container>;

} // namespace kw
