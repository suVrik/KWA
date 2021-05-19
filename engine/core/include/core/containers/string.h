#pragma once

#include "core/memory/memory_resource.h"

#include <string>

namespace kw {

template <typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = MemoryResourceAllocator<CharT>>
using BasicString = std::basic_string<CharT, Traits, Allocator>;

using String = BasicString<char>;

template <typename CharT>
using BasicStringView = std::basic_string_view<CharT>;

using StringView = BasicStringView<char>;

} // namespace kw
