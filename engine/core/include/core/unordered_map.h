#pragma once

#include <memory/allocator_linear.h>
#include <memory/allocator_named.h>

#include <unordered_map>

namespace kw {

template <typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>, typename Allocator = AllocatorNamed<std::pair<const Key, T>>>
using UnorderedMap = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;

template <typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
using UnorderedMapLinear = UnorderedMap<Key, T, Hash, KeyEqual, AllocatorLinear<std::pair<const Key, T>>>;

} // namespace kw
