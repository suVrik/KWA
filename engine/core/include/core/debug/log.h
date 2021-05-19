#pragma once

#include <cstdarg>

namespace kw::Log {

void print(const char* format, ...);

void print_va(const char* format, va_list args);

} // namespace kw::Log
