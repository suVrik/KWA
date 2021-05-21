#pragma once

#include "core/memory/memory_resource.h"

#include <map>

namespace kw {

template <typename Key, typename T, typename Compare = std::less<Key>, typename Allocator = MemoryResourceAllocator<std::pair<const Key, T>>>
using Map = std::map<Key, T, Compare, Allocator>;

} // namespace kw
