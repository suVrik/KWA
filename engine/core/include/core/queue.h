#pragma once

#include <memory/allocator_linear.h>
#include <memory/allocator_named.h>

#include <queue>

namespace kw {

template <typename T, typename Container = std::deque<T, AllocatorNamed<T>>>
using Queue = std::queue<T, Container>;

template <typename T>
using QueueLinear = Queue<T, std::deque<T, AllocatorLinear<T>>>;

} // namespace kw
