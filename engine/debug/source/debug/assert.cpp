#include "debug/assert.h"
#include "debug/debug_utils.h"

#include <cstdarg>
#include <cstdio>

namespace kw::assert_details {

#ifdef KW_DEBUG

static char assert_buffer[2048];

bool assert_handler(const char* expression, bool& skip) {
    snprintf(assert_buffer, sizeof(assert_buffer), "Expression: %s", expression);
    return DebugUtils::show_assert_window(assert_buffer, &skip, 1); // Hide `assert_handler` call.
}

static char message_buffer[1024];

bool assert_handler(const char* expression, bool& skip, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(message_buffer, sizeof(message_buffer), format, args);
    va_end(args);

    snprintf(assert_buffer, sizeof(assert_buffer), "Expression: %s\r\nMessage: %s", expression, message_buffer);

    return DebugUtils::show_assert_window(assert_buffer, &skip, 1); // Hide `assert_handler` call.
}

#else

bool assert_handler(const char* expression, bool& skip) {
    return false;
}

bool assert_handler(const char* expression, bool& skip, const char* format, ...) {
    return false;
}

#endif

} // namespace kw::assert_details
