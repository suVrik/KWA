#pragma once

#include <utility>

namespace kw {

template <typename... T>
using Tuple = std::tuple<T...>;

} // namespace kw
