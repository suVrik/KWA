#pragma once

namespace kw::assert_details {

bool assert_handler(const char* expression, bool& skip);
bool assert_handler(const char* expression, bool& skip, const char* format, ...);

} // namespace kw::assert_details

#ifdef KW_DEBUG
#define KW_ASSERT(expression, ...)                                                      \
do {                                                                                    \
    if (!(expression)) {                                                                \
        static bool skip = false;                                                       \
        if (!skip) {                                                                    \
            if (kw::assert_details::assert_handler(#expression, skip, ##__VA_ARGS__)) { \
                __debugbreak();                                                         \
            }                                                                           \
        }                                                                               \
    }                                                                                   \
} while (false)
#else
#define KW_ASSERT(expression, ...) ((void)0)
#endif
