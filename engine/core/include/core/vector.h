#pragma once

#include <memory/allocator_linear.h>
#include <memory/allocator_named.h>

#include <vector>

namespace kw {

template <typename T, typename Allocator = AllocatorNamed<T>>
using Vector = std::vector<T, Allocator>;

template <typename T>
using VectorLinear = Vector<T, AllocatorLinear<T>>;

} // namespace kw
