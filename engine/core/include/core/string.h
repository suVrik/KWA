#pragma once

#include <memory/allocator_linear.h>
#include <memory/allocator_named.h>

#include <string>

namespace kw {

template <typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = AllocatorNamed<CharT>>
using BasicString = std::basic_string<CharT, Traits, Allocator>;

using String = BasicString<char>;

template <typename CharT>
using BasicStringView = std::basic_string_view<CharT>;

using StringView = BasicStringView<char>;

template <typename CharT, typename Traits = std::char_traits<CharT>>
using BasicStringLinear = BasicString<CharT, Traits, AllocatorLinear<CharT>>;

using StringLinear = BasicStringLinear<char>;

} // namespace kw
