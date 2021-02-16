#pragma once

#include <cstdint>

namespace kw::DebugUtils {

/** Return stacktrace except for the last `hide_calls` calls. */
const char* get_stacktrace(uint32_t hide_calls = 0);

/** Show assert window with the given message, stacktrace and control buttons. */
bool show_assert_window(const char* message, bool* skip, uint32_t hide_calls = 0);

} // namespace kw::DebugUtils
