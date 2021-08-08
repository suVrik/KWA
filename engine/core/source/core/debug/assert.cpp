#include "core/debug/assert.h"
#include "core/debug/debug_utils.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace kw::assert_details {

#ifdef KW_DEBUG

bool assert_handler(const char* expression, bool& skip) {
    bool result = true;

    int length = snprintf(nullptr, 0, "Expression: %s", expression);
    if (length >= 0) {
        char* buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
        if (buffer != nullptr) {
            if (snprintf(buffer, static_cast<size_t>(length) + 1, "Expression: %s", expression) > 0) {
                result = DebugUtils::show_assert_window(buffer, &skip, 1); // Hide `assert_handler` call.
            }
            free(buffer);
        }
    }

    return result;
}

bool assert_handler(const char* expression, bool& skip, const char* format, ...) {
    bool result = true;

    va_list args;
    va_start(args, format);

    int format_length = vsnprintf(nullptr, 0, format, args);
    if (format_length >= 0) {
        char* format_buffer = static_cast<char*>(malloc(static_cast<size_t>(format_length) + 1));
        if (format_buffer != nullptr) {
            if (vsnprintf(format_buffer, static_cast<size_t>(format_length) + 1, format, args) > 0) {
                int length = snprintf(nullptr, 0, "Expression: %s\r\nMessage: %s", expression, format_buffer);
                if (length >= 0) {
                    char* buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
                    if (buffer != nullptr) {
                        if (snprintf(buffer, static_cast<size_t>(length) + 1, "Expression: %s\r\nMessage: %s", expression, format_buffer) > 0) {
                            result = DebugUtils::show_assert_window(buffer, &skip, 1); // Hide `assert_handler` call.
                        }
                        free(buffer);
                    }
                }
            }
            free(format_buffer);
        }
    }

    va_end(args);

    return result;
}

#else

bool assert_handler(const char* expression, bool& skip) {
    return false;
}

bool assert_handler(const char* expression, bool& skip, const char* format, ...) {
    return false;
}

#endif // KW_DEBUG

} // namespace kw::assert_details
