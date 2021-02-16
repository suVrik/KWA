#include "debug/debug_utils.h"

#include <cstdio>

// Wingows.h must be included before DbgHelp.h
#include <Windows.h>
#include <DbgHelp.h>

namespace kw::DebugUtils {

#ifdef KW_DEBUG

constexpr size_t MAX_NAME_LENGTH = 255;

static char stacktrace_buffer[4096];
static char symbol_buffer[sizeof(IMAGEHLP_SYMBOL64) + MAX_NAME_LENGTH];

const char* get_stacktrace(uint32_t hide_calls) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread  = GetCurrentThread();

    CONTEXT context{};
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    SymInitialize(process, nullptr, TRUE);
    SymSetOptions(SYMOPT_LOAD_LINES);

    STACKFRAME64 frame{};
    frame.AddrPC.Offset    = context.Rip;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode   = AddrModeFlat;

    char* current_stacktrace = stacktrace_buffer;

    hide_calls++; // Hide `get_stacktrace` call in the stacktrace too.
    while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
        if (hide_calls-- > 0) {
            continue;
        }

        PCCH symbol_name   = "";
        PCCH file_name     = "";
        DWORD line_number  = 0;
        DWORD displacement = 0;

        auto* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(symbol_buffer);
        symbol->SizeOfStruct  = sizeof(symbol_buffer);
        symbol->MaxNameLength = MAX_NAME_LENGTH;

        if (SymGetSymFromAddr64(process, frame.AddrPC.Offset, nullptr, symbol)) {
            symbol_name = symbol->Name;
        }

        IMAGEHLP_LINE64 line{};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &line)) {
            file_name   = line.FileName;
            line_number = line.LineNumber;
        }

        // Avoid some weird looking stacktrace lines such as `:0 (in BaseThreadInitThunk)` or `:0 (in RtlUserThreadStart)`.
        if (line_number > 0) {
            current_stacktrace += snprintf(current_stacktrace, sizeof(stacktrace_buffer) - (current_stacktrace - stacktrace_buffer), "%s:%d (in %s)\r\n", file_name, line_number, symbol_name);
        }
    }

    SymCleanup(process);

    return stacktrace_buffer;
}

#else

const char* get_stacktrace(uint32_t hide_calls) {
    return "";
}

#endif

} // namespace kw::DebugUtils
