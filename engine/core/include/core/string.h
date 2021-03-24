#pragma once

#include <memory/allocator_linear.h>

#include <string>

namespace kw {

template <typename CharT, typename Traits = std::char_traits<CharT>, typename Allocator = std::allocator<CharT>>
using BasicString = std::basic_string<CharT, Traits, Allocator>;

using String = BasicString<char>;

template <typename CharT>
using BasicStringLinear = BasicString<CharT, std::char_traits<CharT>, AllocatorLinear<CharT>>;

using StringLinear = BasicStringLinear<char>;

} // namespace kw
