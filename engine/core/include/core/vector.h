#pragma once

#include <memory/allocator_linear.h>

#include <vector>

namespace kw {

template <typename T, typename Allocator = std::allocator<T>>
using Vector = std::vector<T, Allocator>;

template <typename T>
using VectorLinear = Vector<T, AllocatorLinear<T>>;

} // namespace kw
