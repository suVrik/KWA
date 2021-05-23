#pragma once

#include <cstdlib>

namespace kw::error_details {

void error_handler(const char* expression);
void error_handler(const char* expression, const char* format, ...);

} // namespace kw::error_details

#define KW_ERROR(expression, ...)                                     \
do {                                                                  \
    if (!(expression)) {                                              \
        kw::error_details::error_handler(#expression, ##__VA_ARGS__); \
        __debugbreak();                                               \
        std::abort();                                                 \
    }                                                                 \
} while (false)
