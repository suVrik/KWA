#include "debug/log.h"

#include <cstdio>
#include <Windows.h>

namespace kw::Log {

static char log_buffer[4096];

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);
    va_end(args);

    OutputDebugStringA(log_buffer);
    OutputDebugStringA("\n");
}

void print_va(const char* format, va_list args) {
    vsnprintf(log_buffer, sizeof(log_buffer), format, args);

    OutputDebugStringA(log_buffer);
    OutputDebugStringA("\n");
}

} // namespace kw::Log
