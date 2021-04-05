#pragma once

#include <memory/memory_resource.h>

#include <unordered_map>

namespace kw {

template <typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>, typename Allocator = MemoryResourceAllocator<std::pair<const Key, T>>>
using UnorderedMap = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

} // namespace kw
