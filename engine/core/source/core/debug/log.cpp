#include "core/debug/log.h"

#include <cstdio>
#include <Windows.h>

namespace kw::Log {

void print(const char* format, ...) {
    va_list args;
    va_start(args, format);
    print_va(format, args);
    va_end(args);
}

void print_va(const char* format, va_list args) {
    char buffer[128];

    int length = vsnprintf(buffer, sizeof(buffer), format, args);
    if (length >= 0) {
        if (length < sizeof(buffer)) {
            OutputDebugStringA(buffer);
        } else {
            char* large_buffer = static_cast<char*>(malloc(static_cast<size_t>(length) + 1));
            if (large_buffer != nullptr) {
                if (vsnprintf(large_buffer, static_cast<size_t>(length) + 1, format, args) >= 0) {
                    OutputDebugStringA(large_buffer);
                }
                free(large_buffer);
            }
        }
        OutputDebugStringA("\n");
    }
}

} // namespace kw::Log
