#pragma once

#include "core/memory/memory_resource.h"

#include <unordered_set>

namespace kw {

template <typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>, typename Allocator = MemoryResourceAllocator<Key>>
using UnorderedSet = std::unordered_set<Key, Hash, KeyEqual, Allocator>;

} // namespace kw
