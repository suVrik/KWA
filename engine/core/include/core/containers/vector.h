#pragma once

#include "core/memory/memory_resource.h"

#include <vector>

namespace kw {

template <typename T, typename Allocator = MemoryResourceAllocator<T>>
using Vector = std::vector<T, Allocator>;

} // namespace kw
