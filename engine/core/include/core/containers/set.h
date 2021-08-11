#pragma once

#include "core/memory/memory_resource.h"

#include <set>

namespace kw {

template <typename Key, typename Compare = std::less<Key>, typename Allocator = MemoryResourceAllocator<Key>>
using Set = std::set<Key, Compare, Allocator>;

} // namespace kw
