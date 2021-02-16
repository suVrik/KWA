#pragma once

namespace kw::error_details {

[[noreturn]] void error_handler(const char* expression);
[[noreturn]] void error_handler(const char* expression, const char* format, ...);

} // namespace kw::error_details

#define KW_ERROR(expression, ...)                                     \
do {                                                                  \
    if (!(expression)) {                                              \
        kw::error_details::error_handler(#expression, ##__VA_ARGS__); \
    }                                                                 \
} while (false)
